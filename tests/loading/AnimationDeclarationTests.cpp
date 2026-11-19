#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/compiler/CompiledProjectValidator.h"
#include "../../engine/runtime/AnimationPlayback.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/runtime/RuntimeObjectBuilder.h"
#include "../../engine/compiler/binary/BinaryReader.h"
#include "../../engine/compiler/binary/BinaryWriter.h"
#include "../../engine/compiler/binary/CompiledProjectCodec.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <functional>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

using namespace flx::test;
using Json = nlohmann::ordered_json;
namespace
{
    int serial = 0;
    std::filesystem::path source(const std::string& declaration)
    {
        auto manifest = createMinimalProject("animation_" + std::to_string(++serial));
        writeFile(manifest.parent_path()/"game/root.json", declaration);
        return manifest;
    }
    Json fixture()
    {
        std::ifstream input(std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/animation/declaration.json");
        Json result; input >> result; return result;
    }
    const ObjectDefinition& root(const CompilationResult& result)
    {
        require(result.success, result.diagnostics.all().empty() ? "compile failed" : result.diagnostics.all().back().message);
        return *result.project.resources.findObject(result.project.rootId);
    }
    bool has(const Diagnostics& diagnostics, DiagnosticCode code)
    {
        for (const auto& d : diagnostics.all()) if (d.code == code) return true;
        return false;
    }
    std::vector<unsigned char> bytes(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary); return {(std::istreambuf_iterator<char>(file)), {}};
    }
    void invalid(const Json& node, DiagnosticCode code = DiagnosticCode::InvalidAnimationDeclaration)
    {
        const auto result = compile(source(node.dump()));
        require(!result.success && has(result.diagnostics, code), "invalid declaration rejected with diagnostic: " + node.dump());
    }
    void replaceDouble(std::vector<unsigned char>& data, const std::string& name, double value)
    {
        auto at = std::search(data.begin(), data.end(), name.begin(), name.end());
        require(at != data.end(), "animation binary name found");
        std::ostringstream out(std::ios::binary); flx::binary::BinaryWriter writer(out); writer.writeF64(value);
        const auto encoded = out.str();
        std::copy(encoded.begin(), encoded.end(), at + name.size());
    }
}
int main()
{
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"partial keys, defaults, inactive keys, palette, bitmap and persist", [] {
            auto node = fixture(); auto result = compile(source(node.dump())); const auto& object = root(result);
            const auto& a = object.animations.at("pulse");
            require(object.hasAnimations && a.hasLoop && !a.loop && a.hasFrom && a.from == AnimationFrom::Current, "explicit presence");
            require(a.keys.size()==3 && a.keys[0].at==0 && a.keys[1].at==0.5 && a.keys[2].at==2, "sort and retain inactive");
            require(a.keys[0].representation && a.keys[0].representation->empty() && !a.keys[0].transform && !a.keys[0].color, "empty representation differs from omission");
            const auto& t = *a.keys[1].transform;
            require(t.offsetX && *t.offsetX==0 && !t.offsetY && *t.rotation==720 && *t.scaleX==-1 && *t.scaleY==0, "component absence and signed/zero values");
            require(a.keys[2].color->r==18 && a.keys[2].representation->at(0).bitmap->width==2, "shared color and bitmap rules");
            require(object.visual.representation[0].persist==RepresentationPersistence::First && object.visual.representation[1].persist==RepresentationPersistence::Last, "base persist retained");
            require(has(result.diagnostics, DiagnosticCode::InactiveAnimationKey) && !has(result.diagnostics,DiagnosticCode::AnimationWithoutActiveKeys), "inactive warning only");
            node["animations"]["pulse"].erase("loop"); node["animations"]["pulse"].erase("from");
            result = compile(source(node.dump())); const auto& defaults=root(result).animations.at("pulse");
            require(!defaults.hasLoop && !defaults.loop && !defaults.hasFrom && defaults.from==AnimationFrom::Base, "omitted defaults retain absence");
            node["animations"]["pulse"]["keys"]=Json::array({{{"at",2},{"color","accent"}}});
            result=compile(source(node.dump())); root(result);
            require(has(result.diagnostics,DiagnosticCode::InactiveAnimationKey) && has(result.diagnostics,DiagnosticCode::AnimationWithoutActiveKeys), "all inactive warnings");
        }},
        {"duration boundary and all four representation families", [] {
            auto node=fixture();
            node["animations"]["pulse"]["loop"]=true;
            node["animations"]["pulse"]["keys"]=Json::array({{{"at",1},{"transform",{{"offset",Json::object()},{"rotation",0}}},{"representation",Json::array({
                {{"primitive","ellipse"},{"size",{{"width","50%"}}}},
                {{"geometry",Json::array({{{"x",0},{"y",1}}})},{"mode","fill"}},
                {{"text","A"},{"fontSize",8}},
                {{"bitmap",{{"pixels",Json::array({Json::array({"accent"})})}}}}
            })}}});
            const auto result=compile(source(node.dump()));const auto& key=root(result).animations.at("pulse").keys[0];
            require(!has(result.diagnostics,DiagnosticCode::InactiveAnimationKey) && !has(result.diagnostics,DiagnosticCode::AnimationWithoutActiveKeys),"duration key is active");
            require(key.representation->size()==4 && key.representation->at(0).size.width.percentage && key.representation->at(3).bitmap,"families use shared visual parser");
            require(key.transform->rotation && !key.transform->offsetX && !key.transform->offsetY,"empty axis does not invent components");
        }},
        {"text font size keeps historical integer contract", [] {
            for(int size:{-1,0,1}) {
                auto node=fixture();
                node["visual"]["representation"]=Json::array({{{"text","base"},{"fontSize",size}}});
                node["animations"]["pulse"]["keys"]=Json::array({{{"at",0},{"representation",Json::array({{{"text","key"},{"fontSize",size}}})}}});
                const auto manifest=source(node.dump());const auto result=compile(manifest);const auto& object=root(result);
                require(object.visual.representation[0].fontSize==size,"base text integer preserved");
                require(object.animations.at("pulse").keys[0].representation->at(0).fontSize==size,"Animation uses same text contract");
                Diagnostics diagnostics;const auto file=manifest.parent_path()/"text.flxc";
                require(CompiledProjectWriter::write(file.string(),result.project,diagnostics),"serialize historical text size");
                const auto loaded=CompiledProjectReader::read(file.string());require(loaded.success,"load historical text size");
                auto o=RuntimeObjectBuilder::build(*loaded.project.resources.findObject(loaded.project.rootId),"text","");
                const auto snapshot=RepresentationPrimitiveBuilder::build(o,WHITE);
                require(snapshot.size()==1&&snapshot[0].fontSize==1,"existing graphical size floor, independent of Animation");
                const auto& definition=*loaded.project.resources.findObject(loaded.project.rootId);
                require(o.animation.play("pulse",definition.animations,AnimationBaseAppearance::capture(o.representation),diagnostics),"play historical text key");
                const auto animated=RepresentationPrimitiveBuilder::build(o,WHITE);
                require(animated.size()==1&&animated[0].text=="key"&&animated[0].fontSize==1,"Animation reuses the same historical text size rule");
            }
        }},
        {"invalid animation structures and components", [] {
            auto base=fixture();
            for(const auto& value: std::vector<Json>{nullptr, false, 1, "bad", Json::array()}) {auto n=base;n["animations"]=value;invalid(n);}
            for(const auto& value: std::vector<Json>{nullptr, true, "1", 0, -1}) {auto n=base;n["animations"]["pulse"]["duration"]=value;invalid(n);}
            for(const auto& value: std::vector<Json>{nullptr, 1, "false"}) {auto n=base;n["animations"]["pulse"]["loop"]=value;invalid(n);}
            for(const auto& value: std::vector<Json>{nullptr, false, "CURRENT", "other"}) {auto n=base;n["animations"]["pulse"]["from"]=value;invalid(n);}
            for(const auto& value: std::vector<Json>{nullptr, Json::array(), 1, Json::object()}) {auto n=base;n["animations"]["pulse"]["keys"]=value;invalid(n);}
            const std::vector<Json> badKeys={Json::object(),{{"at",-1},{"color","red"}},{{"at",true},{"color","red"}},{{"at",0}},{{"at",0},{"transform",Json::object()}},{{"at",0},{"transform",{{"offset",false}}}},{{"at",0},{"transform",{{"rotation",true}}}},{{"at",0},{"transform",{{"scale",{{"x","1"}}}}}},{{"at",0},{"color",false}},{{"at",0},{"representation",false}}};
            for(const auto& key:badKeys) {auto n=base;n["animations"]["pulse"]["keys"]=Json::array({key});invalid(n);}
            auto n=base;n["animations"][""]=n["animations"]["pulse"];invalid(n);
            n=base;n["animations"]["pulse"].erase("duration");invalid(n);
            n=base;n["animations"]["pulse"]["keys"][2]["color"]="unknown-animation-color";invalid(n,DiagnosticCode::UnknownVisualColor);
            n=base;n["visual"]["representation"][0]["persist"]="middle";invalid(n,DiagnosticCode::InvalidVisualPersistence);
            n=base;n["animations"]["pulse"]["keys"][0]["representation"][0]["persist"]="first";invalid(n,DiagnosticCode::InvalidVisualPersistence);
            n=base;n["animations"]["pulse"]["keys"][0]["representation"][0]["bitmap"]["pixels"]=Json::array({"xx","x"});invalid(n,DiagnosticCode::InvalidBitmapDeclaration);
            for(const auto& element:std::vector<Json>{Json{{"text","missing font"}},Json{{"primitive","ellipse"},{"text","two families"},{"fontSize",8}},Json{{"geometry",Json::array()}}})
            {
                n=base;n["animations"]["pulse"]["keys"]=Json::array({{{"at",0},{"representation",Json::array({element})}}});invalid(n);
            }
        }},
        {"duplicates detected before overwrite and before time sorting", [] {
            auto result=compile(source(R"({"animations":{"same":{"duration":1,"keys":[{"at":0,"color":"red"}]},"same":{"duration":2,"keys":[{"at":0,"color":"blue"}]}}})"));
            require(!result.success && has(result.diagnostics,DiagnosticCode::InvalidAnimationDeclaration), "raw duplicate JSON names");
            auto n=fixture();n["animations"]["pulse"]["keys"]=Json::array({{{"at",2},{"color","red"}},{{"at",2.0},{"color","blue"}}});invalid(n);
            n["animations"]["pulse"]["keys"]=Json::array({{{"at",0},{"color","red"}},{{"at",-0.0},{"color","blue"}}});invalid(n);
            result=compile(source(R"({"visual":{"palette":{"a":"red","a":"blue"}},"animations":{"ok":{"duration":1,"keys":[{"at":0,"color":"a"}]}}})"));
            require(root(result).animations.at("ok").keys[0].color->b==BLUE.b,"palette last-wins preserved");
            const auto manifest=source(R"({"like":"./base.json","animations":{"pulse":{"keys":[{"at":0,"color":"red"}]}}})");
            writeFile(manifest.parent_path()/"game/base.json",n.dump());
            result=compile(manifest);
            require(!result.success && has(result.diagnostics,DiagnosticCode::InvalidAnimationDeclaration),"duplicate base times detected before array replacement");
        }},
        {"finite binary64 values are not narrowed or merged", [] {
            const auto result=compile(source(R"({"animations":{"precise":{"duration":1e300,"keys":[{"at":1,"transform":{"rotation":1e300}},{"at":1.0000000000000002,"transform":{"scale":{"x":1e-300}}}]}}})"));
            const auto& a=root(result).animations.at("precise");
            require(a.keys[0].at!=a.keys[1].at && a.duration==1e300,"binary64 model");
            const auto manifest=source("{}"); const auto file=manifest.parent_path()/"precise.flxc";Diagnostics d;
            require(CompiledProjectWriter::write(file.string(),result.project,d),"write finite binary64");
            const auto loaded=CompiledProjectReader::read(file.string());require(loaded.success,"read finite binary64");
            const auto& b=loaded.project.resources.findObject(loaded.project.rootId)->animations.at("precise");
            require(b.duration==a.duration && b.keys[1].at==a.keys[1].at && b.keys[1].transform->scaleX==a.keys[1].transform->scaleX,"exact binary64 roundtrip");
        }},
        {"like overrides, shared resources and Machine projection", [] {
            auto manifest=source("{}");auto game=manifest.parent_path()/"game";
            writeFile(game/"base.json",fixture().dump());
            writeFile(game/"root.json",R"({"children":{"a":{"like":"./base.json","animations":{"pulse":{"duration":3}}},"b":{"like":"./base.json"}}})");
            writeFile(manifest,"name=Animation\npath=game\nroot=root\nmachine=machine.yml\n");
            writeFile(manifest.parent_path()/"machine.yml","machine:\n  video:\n    color:\n      alpha: false\n      palette: [white, black]\n");
            const auto result=compile(manifest);const auto& object=root(result);
            const auto& a=*result.project.resources.findObject(object.childResources.at("a"));
            const auto& b=*result.project.resources.findObject(object.childResources.at("b"));
            require(a.animations.at("pulse").duration==3 && b.animations.at("pulse").duration==1,"consumer override is isolated");
            const auto& key=b.animations.at("pulse").keys[2];
            require(key.color->a==255 && (key.color->r==0 || key.color->r==255),"key target passes through Machine");
            require(key.representation->at(0).bitmap->pixels[0].a==255,"inactive bitmap blank passes through Machine");
        }},
        {"external palettes and representation like retain declaring directory", [] {
            const auto manifest=source("{}");const auto game=manifest.parent_path()/"game";
            auto base=fixture();base["visual"]["palette"]=Json::array({"./colors.json"});
            base["animations"]["pulse"]["keys"]=Json::array({{{"at",0},{"representation",Json::array({{{"like","./element.json"}}})}}});
            writeFile(game/"base/colors.json",R"({"accent":"red"})");
            writeFile(game/"base/element.json",R"({"primitive":"triangle","color":"accent"})");
            writeFile(game/"base/object.json",base.dump());
            writeFile(game/"consumer/element.json",R"({"text":"consumer","fontSize":8,"color":"accent"})");
            writeFile(game/"consumer/colors.json",R"({"accent":"blue"})");
            auto consumer=Json{{"like","../base/object.json"},{"visual",{{"palette",Json::array({"./colors.json"})}}}};
            consumer["animations"]["other"]={{"duration",1},{"keys",Json::array({{{"at",0},{"representation",Json::array({{{"like","./element.json"}}})}}})}};
            writeFile(game/"consumer/object.json",consumer.dump());
            writeFile(game/"root.json",R"({"like":"./consumer/object.json"})");
            auto result=compile(manifest);const auto& object=root(result);
            const auto& inherited=object.animations.at("pulse").keys[0].representation->at(0);
            const auto& overridden=object.animations.at("other").keys[0].representation->at(0);
            require(inherited.kind==RepresentationElementKind::Primitive && inherited.primitive=="triangle","base element resolves at base directory");
            require(overridden.kind==RepresentationElementKind::Text && overridden.text=="consumer","consumer element resolves at consumer directory");
            require(inherited.color.b==BLUE.b && overridden.color.b==BLUE.b,"both consume effective consumer palette");
            writeFile(game/"consumer/element.json",R"({"text":"consumer","fontSize":8,"persist":"first"})");
            result=compile(manifest);require(!result.success && has(result.diagnostics,DiagnosticCode::InvalidVisualPersistence),"persist cannot enter keys through like");
        }},
        {"format14 deterministic autonomous roundtrip and corruption", [] {
            const auto manifest=source(fixture().dump());auto result=compile(manifest);root(result);
            const auto file=manifest.parent_path()/"animation.flxc",second=manifest.parent_path()/"second.flxc";Diagnostics diagnostics;
            require(CompiledProjectWriter::write(file.string(),result.project,diagnostics),"write format14");
            require(CompiledProjectWriter::write(second.string(),result.project,diagnostics) && bytes(file)==bytes(second),"deterministic bytes");
            const auto original=bytes(file);std::filesystem::remove(manifest.parent_path()/"game/root.json");
            const auto loaded=CompiledProjectReader::read(file.string());require(loaded.success && loaded.metadata.formatVersion==14,"autonomous format14 read");
            const auto& object=*loaded.project.resources.findObject(loaded.project.rootId);
            require(object.animations.at("pulse").keys.size()==3 && object.visual.representation[1].persist==RepresentationPersistence::Last,"definitions survive");
            require(CompiledProjectWriter::write(second.string(),loaded.project,diagnostics) && bytes(second)==original,"semantic roundtrip preserves bytes and absence");
            auto broken=original;replaceDouble(broken,"pulse",std::numeric_limits<double>::quiet_NaN());writeBinary(file,broken);
            require(!CompiledProjectReader::read(file.string()).success,"NaN duration rejected defensively");
            broken=original;replaceDouble(broken,"pulse",0);writeBinary(file,broken);require(!CompiledProjectReader::read(file.string()).success,"zero duration rejected");
            broken=original;broken.resize(broken.size()-1);writeBinary(file,broken);require(!CompiledProjectReader::read(file.string()).success,"truncation rejected");
            broken=original;broken.push_back(0);writeBinary(file,broken);require(!CompiledProjectReader::read(file.string()).success,"trailing bytes rejected");
            broken=original;broken[4]=15;writeBinary(file,broken);require(!CompiledProjectReader::read(file.string()).success,"unsupported version rejected");
        }},
        {"writer and in-memory validator reject invalid definitions", [] {
            const auto manifest=source(fixture().dump());const auto valid=compile(manifest);root(valid);
            for(int mutation=0;mutation<7;++mutation)
            {
                auto result=valid;auto object=*result.project.resources.findObject(result.project.rootId);auto& a=object.animations.at("pulse");
                if(mutation==0) a.keys[1].at=a.keys[0].at;
                if(mutation==1) a.duration=std::numeric_limits<double>::infinity();
                if(mutation==2) a.keys[1].transform->rotation=std::numeric_limits<double>::infinity();
                if(mutation==3) a.keys[0].representation->push_back(object.visual.representation[0]);
                if(mutation==4) a.keys[0].representation->push_back(RepresentationElementDefinition{});
                if(mutation==5) a.from=static_cast<AnimationFrom>(255);
                if(mutation==6) object.hasAnimations=false;
                ResourceRegistry resources;require(resources.addObject(result.project.rootId,object),"resource copy");result.project.resources=std::move(resources);
                Diagnostics d;require(!CompiledProjectWriter::write((manifest.parent_path()/"bad.flxc").string(),result.project,d),"invalid compiled model rejected");
                // Bypass the public writer validator to exercise untrusted on-disk models.
                std::ostringstream raw(std::ios::binary); flx::binary::BinaryWriter encoder(raw);
                flx::binary::CompiledProjectCodec::write(encoder,result.project);
                const auto payload=raw.str(); const auto file=manifest.parent_path()/"untrusted.flxc";
                writeBinary(file,{payload.begin(),payload.end()});
                require(!CompiledProjectReader::read(file.string()).success,"reader rejects malformed compiled model independently");
            }
        }},
        {"binary duplicate names, invalid tags, booleans and counts", [] {
            auto node=fixture();node["animations"]["other"]=node["animations"]["pulse"];
            const auto manifest=source(node.dump());auto result=compile(manifest);root(result);
            const auto file=manifest.parent_path()/"mutated.flxc";Diagnostics d;
            require(CompiledProjectWriter::write(file.string(),result.project,d),"write two names");
            const auto original=bytes(file);
            auto broken=original;
            // Use stable string storage for byte searches.
            const std::string pulse="pulse",other="other";
            auto at=std::search(broken.begin(),broken.end(),pulse.begin(),pulse.end());
            require(at!=broken.end(),"name to duplicate found");std::copy(other.begin(),other.end(),at);
            writeBinary(file,broken);auto loaded=CompiledProjectReader::read(file.string());
            require(!loaded.success && has(loaded.diagnostics,DiagnosticCode::DuplicateCompiledEntry),"duplicate binary names rejected before insert");
            for(int mutation=0;mutation<3;++mutation)
            {
                broken=original;at=std::search(broken.begin(),broken.end(),other.begin(),other.end());require(at!=broken.end(),"first name found");
                const auto start=static_cast<size_t>(at-broken.begin())+other.size();
                if(mutation==0) broken[start+8]=2; // loop must be a canonical boolean
                if(mutation==1) broken[start+10]=255; // from enum
                if(mutation==2) std::fill_n(broken.begin()+start+12,4,255); // key count budget
                writeBinary(file,broken);require(!CompiledProjectReader::read(file.string()).success,"malformed animation record rejected");
            }
        }},
        {"CPU evaluator matches source and format14 definitions", [] {
            std::ifstream input(std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/animation/temporal-loop.json");
            Json object;input>>object;const auto manifest=source(object.dump());const auto result=compile(manifest);const auto& open=root(result);
            const auto file=manifest.parent_path()/"temporal.flxc";Diagnostics d;
            require(CompiledProjectWriter::write(file.string(),result.project,d),"write temporal fixture");
            const auto loaded=CompiledProjectReader::read(file.string());require(loaded.success && loaded.metadata.formatVersion==14,"read temporal fixture");
            const auto& binary=*loaded.project.resources.findObject(loaded.project.rootId);
            const auto base=AnimationBaseAppearance::capture(open.visual.representation);
            AnimationPlayback a,b;require(a.play("travel",open.animations,base,d) && b.play("travel",binary.animations,base,d),"play compiled definitions internally");
            std::ifstream expected(std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/animation/temporal-loop.trace.json");Json trace;expected>>trace;
            for(const auto& step:trace["steps"])
            {
                require(a.advance(step["delta"],d) && b.advance(step["delta"],d),"advance both definitions");
                const auto first=a.appearance(base),second=b.appearance(base);
                require(first.transform.offsetX==second.transform.offsetX && first.transform.rotation==second.transform.rotation &&
                    first.transform.scaleX==second.transform.scaleX && a.left()==b.left(),"source and binary evaluate identically");
                require(first.transform.offsetX==step["x"] && first.transform.rotation==step["rotation"] &&
                    first.transform.scaleX==step["scaleX"] && a.left()==step["left"],"compiled fixture matches independent expected trace");
            }
            require(!d.hasErrors(),"evaluation is nonfatal");
        }},
        {"rounded decimal cycle boundary agrees in source and format14", [] {
            std::ifstream input(std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/animation/temporal-loop.json");Json object;input>>object;
            object["animations"]["travel"]["duration"]=0.1;object["animations"]["travel"]["keys"][1]["at"]=0.1;
            const auto manifest=source(object.dump());const auto result=compile(manifest);const auto& open=root(result);
            const auto file=manifest.parent_path()/"decimal.flxc";Diagnostics d;
            require(CompiledProjectWriter::write(file.string(),result.project,d),"write decimal duration");
            const auto loaded=CompiledProjectReader::read(file.string());require(loaded.success,"read decimal duration");
            const auto& binary=*loaded.project.resources.findObject(loaded.project.rootId);
            const auto base=AnimationBaseAppearance::capture({});
            for(const auto* definitions:{&open.animations,&binary.animations})
            {
                AnimationPlayback player;require(player.play("travel",*definitions,base,d) && player.advance(1,d),"evaluate rounded quotient");
                const auto pose=player.appearance(base);
                require(player.left()==0.1 && pose.transform.offsetX==100 && pose.transform.rotation==7200 && pose.transform.scaleX==1,"ten cycles with rounded multiplication and zero remainder");
            }
            require(d.empty(),"decimal boundary is not a numeric failure");
        }},
        {"RuntimeObject owns independent playback without spatial or physics writes", [] {
            ObjectDefinition definition;definition.hasOrigin=true;definition.origin={10,20};definition.hasSize=true;definition.size={30,40};
            definition.mechanics.rotation.angle=35;definition.hasVisual=true;definition.visual.hasColor=false;
            definition.collisions["body"].offset={2,3};definition.collisions["body"].size={4,5,true,true};
            AnimationDefinition a;a.duration=1;AnimationTransformDefinition t;t.offsetX=100;t.offsetY=200;t.rotation=720;t.scaleX=-2;t.scaleY=0;
            AnimationKeyDefinition key;key.at=1;key.transform=t;definition.animations["move"]=a;definition.animations["move"].keys={key};
            auto first=RuntimeObjectBuilder::build(definition,"first","");auto second=RuntimeObjectBuilder::build(definition,"second","");
            first.visible=false;first.state="sentinel";first.stateTime=3;first.velocity={6,7};first.angularVelocity=8;first.depth=9;
            first.timers["timer"]={2,1,RuntimeTimerStatus::Paused};
            const auto base=AnimationBaseAppearance::capture(first.representation,WHITE,first.hasVisualColor);Diagnostics d;
            require(first.animation.play("move",definition.animations,base,d) && first.animation.advance(1,d),"per-instance evaluator");
            require(second.animation.status()==AnimationPlaybackStatus::Absent,"new instance does not inherit playback");
            require(first.position.x==10 && first.position.y==20 && first.origin.x==10 && first.origin.y==20 && first.size.x==30 && first.size.y==40 && first.angle==35,"spatial state unchanged");
            require(first.velocity.x==6 && first.velocity.y==7 && first.angularVelocity==8 && first.collisions.at("body").offset.x==2 && first.collisions.at("body").size.width==4,"physics and colliders unchanged");
            require(!first.visible && first.state=="sentinel" && first.stateTime==3 && first.depth==9 && !first.hasVisualColor && first.timers.at("timer").left==1,"visibility/state/depth/color-presence/timers unchanged");
            require(first.animation.done() && first.animation.appearance(base).transform.rotation==720,"separate evaluated appearance");
        }},
        {"legacy formats12 and13 retain absent Animation", [] {
            for(const auto& name:{"format12-windows.flxc","format13-windows.flxc"})
            {
                const auto loaded=CompiledProjectReader::read((std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures"/name).string());
                require(loaded.success,"read genuine legacy fixture");
                for(const auto& [id,object]:loaded.project.resources.allObjects())
                {
                    require(!object.hasAnimations && object.animations.empty(),"legacy Animation absent");
                    for(const auto& e:object.visual.representation) require(e.persist==RepresentationPersistence::None,"legacy persist absent");
                }
            }
            const auto bitmap=CompiledProjectReader::read((std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/format13-bitmap-windows.flxc").string());
            require(bitmap.success && bitmap.metadata.formatVersion==13,"read actual legacy bitmap layout");
            const auto& e=bitmap.project.resources.findObject(bitmap.project.rootId)->visual.representation.at(0);
            require(e.bitmap && e.bitmap->width==2 && e.bitmap->height==1 && e.bitmap->pixels[0].r==RED.r && e.bitmap->pixels[1].a==0 && e.persist==RepresentationPersistence::None,"legacy bitmap pixels and absence survive");
        }}
    };
    for(const auto& [name, test]:tests)
    {
        try {test();std::cout<<"[PASS] "<<name<<'\n';}
        catch(const std::exception& e){std::cerr<<"[FAIL] "<<name<<": "<<e.what()<<'\n';return 1;}
    }
}
