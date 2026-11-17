#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/compiler/binary/BinaryReader.h"
#include "../../engine/compiler/binary/CompiledProjectCodec.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/runtime/RuntimeObject.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

using namespace flx::test;
using Json = nlohmann::ordered_json;

namespace
{
    int serial = 0;
    std::filesystem::path source(const Json& visual, const std::string& machine = "")
    {
        auto manifest = createMinimalProject("visual_bitmap_" + std::to_string(++serial));
        writeFile(manifest.parent_path() / "game/root.json", Json{{"visual", visual}}.dump());
        if (!machine.empty())
        {
            writeFile(manifest, "name=Bitmap\npath=game\nroot=root\nmachine=machine.yml\n");
            writeFile(manifest.parent_path() / "machine.yml", machine);
        }
        return manifest;
    }
    Json visual(const Json& pixels, const Json& palette = Json::object())
    {
        return Json{{"palette", palette}, {"representation", Json::array({Json{{"bitmap", {{"pixels", pixels}}}}})}};
    }
    const ObjectDefinition& root(const CompilationResult& result)
    {
        require(result.success, result.diagnostics.hasErrors() ? result.diagnostics.all().back().message : "compile failed");
        return *result.project.resources.findObject(result.project.rootId);
    }
    bool equal(Color a, Color b) { return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a; }
    void error(const Json& declaration, DiagnosticCode code)
    {
        const auto result = compile(source(declaration));
        require(!result.success, "invalid visual must fail compilation");
        for (const auto& diagnostic : result.diagnostics.all())
            if (diagnostic.code == code) return;
        throw std::runtime_error("expected specific visual diagnostic: " + declaration.dump());
    }
}

int main()
{
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"compact, explicit, non-square and native-name overrides", [] {
            auto v = visual(Json::array({"01A", "A10"}), {{"0","blank"},{"1","red"},{"A","#123"},{"red","blue"}});
            auto result = compile(source(v));
            auto bitmap = root(result).visual.representation[0].bitmap;
            require(bitmap->width==3 && bitmap->height==2, "dimensions are inferred");
            require(equal(bitmap->pixels[0], BLANK) && equal(bitmap->pixels[1], RED), "entry values use native colors, not aliases");
            require(equal(bitmap->pixels[2], Color{17,34,51,255}), "short hexadecimal");
            v["representation"][0]["bitmap"]["pixels"] = Json::array({Json::array({"red","#FF44AA","blank"})});
            result = compile(source(v));
            bitmap = root(result).visual.representation[0].bitmap;
            require(equal(bitmap->pixels[0], BLUE) && equal(bitmap->pixels[1], Color{255,68,170,255}), "palette precedes native lookup");
        }},
        {"blank override does not change native blank palette values", [] {
            const auto result = compile(source(visual(Json::array({Json::array({"blank","0"})}), {{"blank","red"},{"0","blank"}})));
            const auto& pixels = root(result).visual.representation[0].bitmap->pixels;
            require(equal(pixels[0],RED) && equal(pixels[1],BLANK), "native values are independent of shadowing keys");
        }},
        {"ordered external relative, logical and member palettes are independent", [] {
            auto manifest = source(Json::object());
            const auto game = manifest.parent_path()/"game";
            writeFile(game/"commons/palette.json", R"({"0":"blank","1":"red","visual":"white","like":"blue"})");
            writeFile(game/"commons/colors.json", R"({"arcade":{"1":"green","2":"#FFAA00"}})");
            Json object;
            object["children"]["a"]["visual"] = visual(Json::array({"12"}), Json::array({"./commons/palette.json","./commons/colors.json:arcade",Json{{"1","blue"}}}));
            object["children"]["b"]["visual"] = visual(Json::array({"1"}), Json::array({"/commons/palette"}));
            writeFile(game/"root.json", object.dump());
            const auto result = compile(manifest); const auto& r = root(result);
            const auto& a = *result.project.resources.findObject(r.childResources.at("a"));
            const auto& b = *result.project.resources.findObject(r.childResources.at("b"));
            require(equal(a.visual.representation[0].bitmap->pixels[0],BLUE), "last composition entry wins");
            require(equal(b.visual.representation[0].bitmap->pixels[0],RED), "shared references are not mutated");
            std::ifstream file(game/"commons/palette.json"); Json original; file >> original;
            require(original["1"]=="red", "source map unchanged");
        }},
        {"external visual and inherited consumer preserve palette provenance", [] {
            auto manifest=source(Json::object()); const auto game=manifest.parent_path()/"game";
            writeFile(game/"base/colors.json", R"({"x":"red","y":"green"})");
            writeFile(game/"base/visual.json", visual(Json::array({"x"}),Json::array({"./colors.json"})).dump());
            writeFile(game/"base/object.json", R"({"visual":"./visual.json"})");
            writeFile(game/"consumer/colors.json", R"({"x":"blue"})");
            writeFile(game/"consumer/object.json", R"({"like":"../base/object.json","visual":{"palette":["./colors.json"]}})");
            writeFile(game/"root.json", R"({"like":"./consumer/object.json"})");
            auto result=compile(manifest);
            require(equal(root(result).visual.representation[0].bitmap->pixels[0],BLUE), "consumer override resolves relative to consumer");
        }},
        {"logical member inheritance resolves base palette before consumer overrides", [] {
            const auto manifest=source(Json::object());const auto game=manifest.parent_path()/"game";
            writeFile(game/"base/colors.json", R"({"x":"red","y":"green"})");
            Json base; base["sprite"]["visual"]=visual(Json::array({"xy"}),Json::array({"./colors.json"}));
            writeFile(game/"base/objects.json",base.dump());
            writeFile(game/"root.json",R"({"like":"/base/objects:sprite","visual":{"palette":[{"x":"blue"}]}})");
            const auto result=compile(manifest);const auto bitmap=root(result).visual.representation[0].bitmap;
            require(equal(bitmap->pixels[0],BLUE) && equal(bitmap->pixels[1],GREEN),"base member palette retains inherited keys");
        }},
        {"palette colors apply to visual and every existing representation family", [] {
            Json v{{"palette",{{"accent","#FA0"}}},{"color","accent"},{"representation",Json::array({
                Json{{"primitive","rectangle"},{"color","accent"}},
                Json{{"geometry",Json::array({Json{{"x",0},{"y",0}}})},{"color","accent"}},
                Json{{"text","A"},{"fontSize",8},{"color","accent"}}})}};
            const auto result=compile(source(v)); const auto& r=root(result);
            require(equal(r.visual.color, Color{255,170,0,255}), "visual alias resolves");
            for (const auto& e:r.visual.representation) require(equal(e.color,r.visual.color), "representation alias resolves");
        }},
        {"combined horizontal and vertical repetitions normalize before validation", [] {
            Json rows=Json::array({Json{{"row",Json::array({Json{{"pattern","01"},{"repeat",2}},Json{{"pattern",Json::array({"red","blue"})}}})},{"repeat",3}}});
            const auto result=compile(source(visual(rows,{{"0","blank"},{"1","green"}})));
            const auto bitmap=root(result).visual.representation[0].bitmap;
            require(bitmap->width==6 && bitmap->height==3 && bitmap->pixels.size()==18, "repeated dimensions");
            require(equal(bitmap->pixels[0],BLANK) && equal(bitmap->pixels[4],RED) && equal(bitmap->pixels[17],BLUE), "repetition preserves colors and order");
        }},
        {"Unicode compact cells count characters rather than bytes", [] {
            const std::string key="\xc3\xb1";
            const auto result=compile(source(visual(Json::array({key+key}),Json{{key,"red"}})));
            require(root(result).visual.representation[0].bitmap->width==2, "UTF-8 width");
        }},
        {"Machine alpha and palette constraints include every blank pixel", [] {
            auto v=visual(Json::array({Json::array({"blank","#FF0000"})}));
            const auto alpha=compile(source(v,"machine:\n  video:\n    color:\n      alpha: true\n"));
            require(root(alpha).visual.representation[0].bitmap->pixels[0].a==0,"alpha allowed");
            const auto opaque=compile(source(v,"machine:\n  video:\n    color:\n      alpha: false\n"));
            require(equal(root(opaque).visual.representation[0].bitmap->pixels[0],BLACK),"blank becomes opaque via Machine");
            const auto limited=compile(source(v,"machine:\n  video:\n    color:\n      alpha: false\n      palette: [white, black]\n"));
            for (Color pixel:root(limited).visual.representation[0].bitmap->pixels)
                require(pixel.a==255 && (equal(pixel,WHITE)||equal(pixel,BLACK)),"Machine constrains actual colors");
        }},
        {"unknown colors, palette aliases and references fail specifically", [] {
            error(Json{{"color","unknown-color"}},DiagnosticCode::UnknownVisualColor);
            error(Json{{"representation",Json::array({Json{{"text","A"},{"fontSize",8},{"color","#FFFF"}}})}},DiagnosticCode::UnknownVisualColor);
            error(visual(Json::array({"1"}),{{"primary","red"},{"secondary","primary"}}),DiagnosticCode::InvalidVisualPalette);
            error(visual(Json::array({"1"}),{{"1",false}}),DiagnosticCode::InvalidVisualPalette);
            error(visual(Json::array({"1"})),DiagnosticCode::UnknownVisualColor);
            error(visual(Json::array({Json::array({"none"})})),DiagnosticCode::UnknownVisualColor);
            error(visual(Json::array({"1"}),Json::array({"./missing.json"})),DiagnosticCode::MissingReferencedResource);
            auto manifest=source(visual(Json::array({"1"}),Json::array({"./colors.json:missing"})));
            writeFile(manifest.parent_path()/"game/colors.json",R"({"other":{"1":"red"}})");
            auto result=compile(manifest); require(!result.success,"missing palette member fails");
            bool found=false;for(const auto& d:result.diagnostics.all()) found|=d.code==DiagnosticCode::MissingInternalResourceMember;
            require(found,"existing member diagnostic reused");
        }},
        {"malformed matrices, recursive patterns and excessive expansions fail", [] {
            const std::vector<Json> invalid = {
                Json::array(), Json::array({""}), Json::array({"1","11"}), Json::array({1}),
                Json::array({Json{{"row","1"},{"repeat",0}}}),
                Json::array({Json{{"row","1"},{"repeat",-1}}}),
                Json::array({Json{{"row","1"},{"repeat",1.5}}}),
                Json::array({Json{{"row","1"},{"repeat",true}}}),
                Json::array({Json{{"row","1"},{"repeat",256}}}),
                Json::array({Json{{"row","1"},{"repeat",UINT64_MAX}}}),
                Json::array({std::string(256,'1')}),
                Json::array({Json{{"row",Json::array({Json{{"pattern",Json::array({Json{{"pattern","1"}}})}}})}}}),
                Json::array({Json{{"row",Json::array({Json{{"pattern","11"},{"repeat",128}}})}}}),
                Json::array({Json{{"row","1"},{"repeat",255}},"1"}),
                Json::array({Json{{"row","1"},{"other",1}}})};
            for(const auto& rows:invalid) error(visual(rows,{{"1","red"}}),DiagnosticCode::InvalidBitmapDeclaration);
        }},
        {"maximum bitmap and compiled format 13 roundtrip stay bounded", [] {
            auto v=visual(Json::array({Json{{"row",std::string(255,'1')},{"repeat",255}}}),{{"1","#A1B2C3"}});
            auto manifest=source(v);auto result=compile(manifest);
            require(root(result).visual.representation[0].bitmap->pixels.size()==65025,"255 by 255 accepted");
            const auto file=manifest.parent_path()/"bitmap.flxc";Diagnostics diagnostics;
            require(CompiledProjectWriter::write(file.string(),result.project,diagnostics),"maximum bitmap writes with aggregate payload budget");
            auto read=CompiledProjectReader::read(file.string());
            require(read.success && read.metadata.formatVersion==13,"format 13 roundtrip");
            const auto bitmap=read.project.resources.findObject(read.project.rootId)->visual.representation[0].bitmap;
            require(bitmap->width==255 && bitmap->height==255 && equal(bitmap->pixels.back(),Color{161,178,195,255}),"pixels survive codec");
            std::ifstream input(file,std::ios::binary);std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});
            bytes[4]=12;writeBinary(file,bytes);require(!CompiledProjectReader::read(file.string()).success,"format 12 must reject a bitmap tag");
            bytes[4]=13;bytes.resize(bytes.size()-1);writeBinary(file,bytes);require(!CompiledProjectReader::read(file.string()).success,"truncated bitmap/project rejected");
            auto broken=std::make_shared<BitmapDefinition>();broken->width=2;broken->height=2;broken->pixels={RED};
            auto object=*result.project.resources.findObject(result.project.rootId);object.visual.representation[0].bitmap=broken;
            ResourceRegistry resources;require(resources.addObject(result.project.rootId,object),"invalid object fixture");result.project.resources=std::move(resources);
            Diagnostics invalid;require(!CompiledProjectWriter::write(file.string(),result.project,invalid),"writer validates pixel count");
            std::string length(4,'\xff');std::istringstream stream(length);flx::binary::BinaryReader reader(stream);
            bool rejected=false;try{reader.readBlob(4,"bitmap.rgba");}catch(const flx::binary::BinaryException&){rejected=true;}
            require(rejected,"untrusted payload length rejected before allocation");
        }},
        {"builder preserves composition, transformations, natural size and no tint", [] {
            auto result=compile(source(visual(Json::array({"11"}),{{"1","red"}})));
            RuntimeObject object("bitmap", {0,0}, {0,0}, WHITE);object.hasSize=false;object.position={12,14};object.angle=37;object.hasVisualColor=true;object.visualColor=GREEN;
            object.representation=root(result).visual.representation;
            auto primitives=RepresentationPrimitiveBuilder::build(object,BLUE);
            require(primitives.size()==1 && primitives[0].size.x==2 && primitives[0].size.y==1,"natural size");
            require(primitives[0].a.x==12 && primitives[0].angle==37 && equal(primitives[0].color,WHITE),"transform and no tint");
            object.representation[0].size.width={0.5f,true,true};
            primitives=RepresentationPrimitiveBuilder::build(object,BLUE);
            require(primitives[0].size.x==1 && primitives[0].size.y==1,"percentages use natural size when object has no size");
            object.hasSize=true;object.size={40,20};object.representation[0].size.width={0.5f,true,true};
            primitives=RepresentationPrimitiveBuilder::build(object,BLUE);
            require(primitives[0].size.x==20 && primitives[0].size.y==20,"percentage override and inherited dimension");
            RepresentationElementDefinition text;text.kind=RepresentationElementKind::Text;text.text="A";text.fontSize=8;
            object.representation.insert(object.representation.begin(),text);object.representation.push_back(text);
            primitives=RepresentationPrimitiveBuilder::build(object,BLUE);
            require(primitives.size()==3 && primitives[0].primitive=="text" && primitives[1].bitmap && primitives[2].primitive=="text","composition order");
        }}
    };
    for(const auto& test:tests)
    {
        try{test.second();std::cout<<"[PASS] "<<test.first<<'\n';}
        catch(const std::exception& e){std::cerr<<"[FAIL] "<<test.first<<": "<<e.what()<<'\n';return 1;}
    }
}
