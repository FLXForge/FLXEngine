#include "../support/TestSupport.h"
#include "../../engine/graphics/DeclarativeGeometry.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/graphics/DrawingRenderer.h"
#include "../../engine/graphics/BitmapTextureCache.h"
#include "../../engine/runtime/RuntimeObject.h"
#include "../../engine/runtime/RuntimeObjectBuilder.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/core/FlxSession.h"
#include "../../engine/graphics/DrawingContext.h"
#include <rlgl.h>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <limits>

using namespace flx::test;
namespace
{
    bool equal(Color a,Color b){return a.r==b.r&&a.g==b.g&&a.b==b.b&&a.a==b.a;}
    void near(double a,double b,const char* message){require(std::isfinite(a)&&std::abs(a-b)<0.0001,message);}
    RepresentationElementDefinition shape(std::string kind="rectangle")
    {
        RepresentationElementDefinition e;e.kind=RepresentationElementKind::Primitive;e.primitive=kind;return e;
    }
    RepresentationElementDefinition text(std::string value="A")
    {
        RepresentationElementDefinition e;e.kind=RepresentationElementKind::Text;e.text=value;e.fontSize=10;return e;
    }
    RepresentationElementDefinition bitmap()
    {
        auto pixels=std::make_shared<BitmapDefinition>();pixels->width=pixels->height=2;pixels->pixels={RED,GREEN,BLUE,YELLOW};
        RepresentationElementDefinition e;e.kind=RepresentationElementKind::Bitmap;e.bitmap=pixels;return e;
    }
    RepresentationElementDefinition geometry()
    {
        RepresentationElementDefinition e;e.kind=RepresentationElementKind::Geometry;e.geometry={{-6,-6},{6,-6},{6,6},{-6,6}};e.geometryMode="fill";return e;
    }
    RuntimeObject object(RepresentationElementDefinition element)
    {
        RuntimeObject o("animated",{32,32},{16,16},WHITE);o.hasSize=true;o.representation={element};return o;
    }
    void play(RuntimeObject& o,AnimationTransformDefinition t,Diagnostics& d,
        std::optional<Color> color={},std::optional<AnimationRepresentation> rep={},AnimationFrom from=AnimationFrom::Base,std::string name="pose")
    {
        AnimationDefinition a;a.duration=1;a.from=from;a.hasFrom=true;
        AnimationKeyDefinition k;k.at=0;k.transform=t;k.color=color;k.representation=rep;a.keys={k};
        const auto base=AnimationBaseAppearance::capture(o.representation,o.hasVisualColor?o.visualColor:WHITE,o.hasVisualColor);
        require(o.animation.play(name,{{name,a}},base,d),"internal play");
    }
    int ink(Image image)
    {
        int count=0;for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x)
            if(!equal(GetImageColor(image,x,y),BLACK))++count;
        return count;
    }
    bool imagesEqual(Image a,Image b)
    {
        if(a.width!=b.width||a.height!=b.height)return false;
        for(int y=0;y<a.height;++y)for(int x=0;x<a.width;++x)if(!equal(GetImageColor(a,x,y),GetImageColor(b,x,y)))return false;
        return true;
    }
    Image referenceText(const std::string& value,float sx,float sy)
    {
        auto target=LoadRenderTexture(64,64);require(target.id!=0,"reference target");
        BeginTextureMode(target);ClearBackground(BLACK);
        rlDrawRenderBatchActive();rlDisableBackfaceCulling();
        rlPushMatrix();rlTranslatef(32,32,0);rlScalef(sx,sy,1);rlTranslatef(-32,-32,0);
        // Independent reference: the unchanged historical text route under an affine matrix.
        auto original=object(text(value));
        DrawingRenderer::render(RepresentationPrimitiveBuilder::build(original,WHITE),{},WorldExtent{0,0,64,64},64,64);
        rlPopMatrix();rlDrawRenderBatchActive();rlEnableBackfaceCulling();
        EndTextureMode();Image image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);UnloadRenderTexture(target);return image;
    }

    Image raster(RuntimeObject& o,Diagnostics& d,BitmapTextureCache& cache,bool validated=true,float worldAngle=0)
    {
        o.angle=worldAngle;
        auto target=LoadRenderTexture(64,64);require(target.id!=0,"real render texture");
        const auto context=RepresentationRenderContext{{0,0,64,64},64,64,GetFontDefault()};
        auto primitives=validated?RepresentationPrimitiveBuilder::build(o,WHITE,context,d):RepresentationPrimitiveBuilder::build(static_cast<const RuntimeObject&>(o),WHITE);
        BeginTextureMode(target);ClearBackground(BLACK);
        DrawingRenderer::render(primitives,{},context.world,64,64,&cache);
        EndTextureMode();Image result=LoadImageFromTexture(target.texture);ImageFlipVertical(&result);UnloadRenderTexture(target);return result;
    }
}

void testAnimationGeometryCPU()
{
    Diagnostics d;
    auto o=object(geometry());o.angle=90;
    AnimationTransformDefinition t;t.scaleX=-2;t.scaleY=0.5;t.rotation=90;t.offsetX=3;t.offsetY=4;
    play(o,t,d);
    const auto original=o.position;
    auto primitives=RepresentationPrimitiveBuilder::build(static_cast<const RuntimeObject&>(o),WHITE);
    Vector2 p;require(DeclarativeGeometry::worldPoint(primitives[0],1,2,p),"finite CPU point");
    // (-2,1) -> (-1,-2) -> (2,2) -> (-2,2) + (32,32).
    near(p.x,30,"normative x transform order");near(p.y,34,"normative y transform order");
    require(o.position.x==original.x&&o.angle==90&&o.size.x==16,"spatial state untouched");
    const RepresentationRenderContext context{{0,0,64,64},128,64,{}};
    auto prepared=RepresentationPrimitiveBuilder::build(o,WHITE,context,d);
    require(prepared.size()==1&&prepared[0].mesh,"CPU prepared mesh");
    near(prepared[0].mesh->points[0].x,32,"world rotation precedes anisotropic raster x");
    near(prepared[0].mesh->points[0].y,38,"anisotropic raster y");

    GlyphInfo glyphs[2]{};Rectangle recs[2]={{0,0,3,6},{4,0,3,6}};
    glyphs[0].value='?';glyphs[1].value='A';glyphs[0].advanceX=glyphs[1].advanceX=4;
    Font synthetic{};synthetic.baseSize=8;synthetic.glyphCount=2;synthetic.glyphs=glyphs;synthetic.recs=recs;synthetic.texture={1,16,16,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    auto letters=object(text("A\nAA"));letters.representation[0].fontSize=8;
    AnimationTransformDefinition fontTransform;fontTransform.scaleX=-2;fontTransform.scaleY=0.5;fontTransform.offsetX=3;fontTransform.offsetY=4;play(letters,fontTransform,d);
    auto layout=RepresentationPrimitiveBuilder::build(letters,WHITE,{{0,0,64,64},64,64,synthetic},d);
    require(layout.size()==1&&layout[0].mesh->glyphs.size()==3,"CPU text geometry from supplied metrics without graphics context");
    near(layout[0].mesh->glyphs[0].points[0].x,39,"glyph centering under mirror");
    near(layout[0].mesh->glyphs[0].points[0].y,31.5,"multiline block centering");
    near(layout[0].mesh->glyphs[1].points[0].x-layout[0].mesh->glyphs[2].points[0].x,10,"scaled glyph advance and spacing");
    letters.representation[0].text=std::string("A\0AA",4);letters.animation.stop();play(letters,fontTransform,d);
    layout=RepresentationPrimitiveBuilder::build(letters,WHITE,{{0,0,64,64},64,64,synthetic},d);
    require(layout[0].mesh->glyphs.size()==1,"preserve historical embedded-NUL text termination");
    letters.representation[0].text="A";letters.angle=90;
    layout=RepresentationPrimitiveBuilder::build(letters,WHITE,{{0,0,64,64},128,64,synthetic},d);
    near(layout[0].mesh->glyphs[0].points[0].x,58.25,"text local transform before anisotropic projection");
    near(layout[0].mesh->glyphs[0].points[0].y,39,"text world rotation and offset keep historical metrics");

    auto base=shape();base.persist=RepresentationPersistence::First;auto last=shape();last.persist=RepresentationPersistence::Last;last.hasColor=true;last.color=GREEN;
    o=object(shape());o.representation={shape(),last,base};o.hasVisualColor=true;o.visualColor=RED;
    auto absent=RepresentationPrimitiveBuilder::build(o,WHITE);
    require(absent.size()==3&&equal(absent[2].color,GREEN),"persist order in ABSENT");
    AnimationTransformDefinition identity;identity.rotation=0;play(o,identity,d,BLUE,AnimationRepresentation{});
    auto changed=RepresentationPrimitiveBuilder::build(o,WHITE);
    require(changed.size()==2&&equal(changed[0].color,BLUE)&&equal(changed[1].color,GREEN),"empty section and explicit color");
    o.visualColor=YELLOW;changed=RepresentationPrimitiveBuilder::build(o,WHITE);require(equal(changed[0].color,BLUE),"animation color priority");
    o.animation.stop();changed=RepresentationPrimitiveBuilder::build(o,WHITE);require(changed.size()==3&&equal(changed[0].color,YELLOW),"stop returns live runtime appearance");

    o=object(shape());AnimationTransformDefinition previous;previous.offsetX=5;previous.rotation=25;previous.scaleX=2;
    play(o,previous,d,RED,AnimationRepresentation{bitmap()},AnimationFrom::Base,"previous");
    require(RepresentationPrimitiveBuilder::build(o,WHITE,context,d).size()==1,"accept previous graphics");
    AnimationTransformDefinition tooLarge;tooLarge.scaleX=1e300;
    play(o,tooLarge,d,{}, {},AnimationFrom::Current,"new");
    for(int i=0;i<4;++i)
    {
        auto recovered=RepresentationPrimitiveBuilder::build(o,WHITE,context,d);
        require(recovered.size()==1&&recovered[0].primitive=="rectangle"&&!recovered[0].bitmap,"backup cannot retain uncontrolled representation");
        require(equal(recovered[0].color,WHITE),"backup cannot retain uncontrolled color");
        near(recovered[0].visualTransform.rotation,0,"backup cannot retain uncontrolled rotation");
        near(recovered[0].visualTransform.offsetX,0,"backup cannot retain uncontrolled offset");
        near(recovered[0].visualTransform.scaleX,2,"last graphically valid controlled scale");
    }
    size_t count=0;for(const auto& diagnostic:d.all())count+=diagnostic.code==DiagnosticCode::AnimationNumericRecovery;
    require(count==1&&!d.hasErrors(),"one recoverable diagnostic, not per frame");
    near(o.animation.elapsed(),0,"render does not advance time");
    o.animation.stop();auto stopped=RepresentationPrimitiveBuilder::build(o,WHITE,context,d);
    near(stopped[0].visualTransform.scaleX,1,"stop removes numeric backup");
    Diagnostics recoveryDiagnostics;auto recovering=object(shape());
    play(recovering,previous,recoveryDiagnostics,{}, {},AnimationFrom::Base,"accepted");
    RepresentationPrimitiveBuilder::build(recovering,WHITE,context,recoveryDiagnostics);
    AnimationDefinition recovery;recovery.duration=1;recovery.from=AnimationFrom::Current;recovery.hasFrom=true;
    AnimationKeyDefinition start,finish;start.at=0;start.transform=tooLarge;finish.at=1;finish.transform.emplace();finish.transform->scaleX=0.5;recovery.keys={start,finish};
    const auto appearanceBase=AnimationBaseAppearance::capture(recovering.representation);
    require(recovering.animation.play("recovery",{{"recovery",recovery}},appearanceBase,recoveryDiagnostics),"new recovery trajectory");
    recovering.animation.pause();auto retained=RepresentationPrimitiveBuilder::build(recovering,WHITE,context,recoveryDiagnostics);
    require(recovering.animation.paused()&&recovering.animation.elapsed()==0,"graphics recovery does not resume or advance playback");
    near(retained[0].visualTransform.scaleX,2,"paused graphics fallback");
    require(recovering.animation.play("recovery",{{"recovery",recovery}},appearanceBase,recoveryDiagnostics)&&recovering.animation.advance(1,recoveryDiagnostics),"explicit internal advance recovers");
    auto validAgain=RepresentationPrimitiveBuilder::build(recovering,WHITE,context,recoveryDiagnostics);
    near(validAgain[0].visualTransform.scaleX,1,"DONE validates the recovered final pose");
    require(recovering.animation.done()&&recoveryDiagnostics.size()==1,"graphic recovery shares per-playback diagnostic suppression");

}

void testAnimationRendering()
{
    const bool own=!IsWindowReady();
    if(own){SetTraceLogLevel(LOG_WARNING);SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(64,64,"Animation pixel regression");}
    require(IsWindowReady(),"real graphics context required");
    {
        BitmapTextureCache cache;Diagnostics d;
        AnimationTransformDefinition identity;identity.rotation=0;
        for(auto element:{shape(),shape("triangle"),shape("ellipse"),geometry(),text("Mi\nA"),bitmap()})
        {
            auto o=object(element);
            Image baseline=raster(o,d,cache);play(o,identity,d);Image unchanged=raster(o,d,cache);
            require(imagesEqual(baseline,unchanged),"identity animation preserves historical pixels for every family");
            require(ink(baseline)>0,"baseline contains visible pixels");
            UnloadImage(baseline);UnloadImage(unchanged);
            AnimationTransformDefinition scale;scale.scaleX=-2;scale.scaleY=0.5;scale.offsetX=5;
            play(o,scale,d,{}, {},AnimationFrom::Base,"mirror");Image transformed=raster(o,d,cache);
            require(ink(transformed)>0,"nonuniform negative scale remains visible for every family");
            UnloadImage(transformed);
            scale.scaleX=scale.scaleY=0;play(o,scale,d,{}, {},AnimationFrom::Base,"collapse");Image zero=raster(o,d,cache);
            require(ink(zero)==0,"zero scale emits no filled pixels");UnloadImage(zero);
        }
        for(const auto& scale:std::vector<Vector2>{{-2,0.5f},{1,-1},{2,1},{-1,-1}})
        {
            const std::string value="A B\tC\n\xc3\x91?";
            auto letters=object(text(value));AnimationTransformDefinition affine;affine.scaleX=scale.x;affine.scaleY=scale.y;
            play(letters,affine,d);Image actual=raster(letters,d,cache);Image expected=referenceText(value,scale.x,scale.y);
            require(imagesEqual(actual,expected),"text matches independent historical metrics/spacing/multiline/UTF8 affine reference");
            UnloadImage(actual);UnloadImage(expected);
        }
        auto combined=object(bitmap());AnimationTransformDefinition affine;affine.scaleX=-2;affine.scaleY=0.5;affine.rotation=90;affine.offsetX=3;affine.offsetY=4;
        play(combined,affine,d);Image mapped=raster(combined,d,cache,true,90);
        require(equal(GetImageColor(mapped,20,37),RED)&&equal(GetImageColor(mapped,36,37),GREEN)&&equal(GetImageColor(mapped,20,33),BLUE),"combined scale/rotation/offset/world rotation preserves observable bitmap quadrants");
        UnloadImage(mapped);
        auto cellDefinition=geometry();cellDefinition.geometry={{0,0}};
        auto cell=object(cellDefinition);AnimationTransformDefinition enlarged;enlarged.scaleX=2;enlarged.scaleY=3;play(cell,enlarged,d);
        Image cellFrame=raster(cell,d,cache);require(ink(cellFrame)==6&&equal(GetImageColor(cellFrame,33,34),WHITE),"one-point geometry transforms its historical cell area");UnloadImage(cellFrame);
        enlarged.scaleX=enlarged.scaleY=0;play(cell,enlarged,d,{}, {},AnimationFrom::Base,"zero-cell");cellFrame=raster(cell,d,cache);
        require(ink(cellFrame)==0,"one-point geometry collapses under zero scale");UnloadImage(cellFrame);
        auto longLine=geometry();longLine.geometryMode="open";longLine.geometry={{-1e30f,0},{1e30f,0}};
        auto lineObject=object(longLine);AnimationTransformDefinition lineOffset;lineOffset.offsetY=1;play(lineObject,lineOffset,d);
        Image lineImage=raster(lineObject,d,cache);
        auto lineTarget=LoadRenderTexture(64,64);BeginTextureMode(lineTarget);ClearBackground(BLACK);DrawLineEx({0,33},{64,33},1,WHITE);EndTextureMode();
        Image lineReference=LoadImageFromTexture(lineTarget.texture);ImageFlipVertical(&lineReference);UnloadRenderTexture(lineTarget);
        require(imagesEqual(lineImage,lineReference)&&ink(lineImage)==64,"large finite line clips without losing the visible stroke to cancellation");
        UnloadImage(lineImage);UnloadImage(lineReference);
        auto largeFill=geometry();largeFill.geometry={{-1e8f,-1e8f},{1e8f,-1e8f},{1e8f,1e8f},{-1e8f,1e8f}};
        auto fillObject=object(largeFill);play(fillObject,identity,d,RED);Image fillImage=raster(fillObject,d,cache);
        require(ink(fillImage)==64*64&&equal(GetImageColor(fillImage,32,32),RED),"large identity fill clips scan work while preserving all visible pixels");UnloadImage(fillImage);
        auto o=object(bitmap());AnimationTransformDefinition flip;flip.scaleX=-1;play(o,flip,d);
        Image horizontal=raster(o,d,cache);
        require(equal(GetImageColor(horizontal,27,27),GREEN)&&equal(GetImageColor(horizontal,37,27),RED),"horizontal mirror preserves bitmap UV and colors");
        UnloadImage(horizontal);flip.scaleX=1;flip.scaleY=-1;play(o,flip,d,{}, {},AnimationFrom::Base,"vertical");
        Image vertical=raster(o,d,cache);require(equal(GetImageColor(vertical,27,27),BLUE),"vertical mirror preserves UV");UnloadImage(vertical);
        const auto texture=cache.get(o.representation[0].bitmap);
        for(int i=0;i<4;++i){Image frame=raster(o,d,cache);UnloadImage(frame);require(cache.get(o.representation[0].bitmap).id==texture.id,"cached texture reused across frames");}
        o=object(shape());play(o,identity,d,RED);Image colored=raster(o,d,cache);
        require(equal(GetImageColor(colored,32,32),RED),"inherited animation color reaches raster");UnloadImage(colored);
        o.visualColor=BLUE;o.hasVisualColor=true;o.animation.stop();colored=raster(o,d,cache);
        require(equal(GetImageColor(colored,32,32),BLUE),"stop restores current base color in raster");UnloadImage(colored);
        // Static persist is already executable without a pending JavaScript API.
        auto grouped=object(shape());auto first=shape();first.persist=RepresentationPersistence::First;first.hasColor=true;first.color=BLUE;
        auto last=shape();last.persist=RepresentationPersistence::Last;last.hasColor=true;last.color=GREEN;last.size.width={4,false,true};last.size.height={4,false,true};
        auto middle=shape();middle.hasColor=true;middle.color=RED;middle.size.width={8,false,true};middle.size.height={8,false,true};
        grouped.representation={middle,last,first};Image groups=raster(grouped,d,cache);
        require(equal(GetImageColor(groups,32,32),GREEN)&&equal(GetImageColor(groups,35,32),RED)&&equal(GetImageColor(groups,38,32),BLUE),"persist order is visible even in ABSENT");UnloadImage(groups);
        play(grouped,identity,d,{},AnimationRepresentation{});groups=raster(grouped,d,cache);
        require(equal(GetImageColor(groups,35,32),BLUE)&&equal(GetImageColor(groups,32,32),GREEN),"empty replacement preserves first and last pixels");UnloadImage(groups);
        // A declarative snapshot does not change after playback mutations.
        auto snapshot=RepresentationPrimitiveBuilder::build(grouped,WHITE,{{0,0,64,64},64,64,GetFontDefault()},d);
        grouped.animation.stop();auto target=LoadRenderTexture(64,64);BeginTextureMode(target);ClearBackground(BLACK);
        DrawingRenderer::render(snapshot,{},WorldExtent{0,0,64,64},64,64,&cache);EndTextureMode();
        groups=LoadImageFromTexture(target.texture);ImageFlipVertical(&groups);UnloadRenderTexture(target);
        require(equal(GetImageColor(groups,35,32),BLUE),"old snapshot survives stop before drawing");UnloadImage(groups);
        // An immediate local pixel keeps the spatial transform, not Animation's offset.
        auto immediate=object(shape());AnimationTransformDefinition shifted;shifted.offsetX=16;play(immediate,shifted,d,BLUE);
        auto primitives=RepresentationPrimitiveBuilder::build(immediate,WHITE,{{0,0,64,64},64,64,GetFontDefault()},d);
        DrawingContext drawing;drawing.begin(immediate,primitives);require(drawing.emitLocalPixel(immediate,{0,0},RED),"emit immediate local pixel");drawing.end();
        target=LoadRenderTexture(64,64);BeginTextureMode(target);ClearBackground(BLACK);DrawingRenderer::render(primitives,{},WorldExtent{0,0,64,64},64,64,&cache);EndTextureMode();
        groups=LoadImageFromTexture(target.texture);ImageFlipVertical(&groups);UnloadRenderTexture(target);
        require(equal(GetImageColor(groups,32,32),RED)&&equal(GetImageColor(groups,48,32),BLUE),"immediate drawing is not animated");UnloadImage(groups);
        const auto manifest=createMinimalProject("animation_renderer_equivalence");
        std::ifstream source(std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/animation/renderer-object.json");std::ostringstream contents;contents<<source.rdbuf();
        writeFile(manifest.parent_path()/"game/root.json",contents.str());const auto compiled=compile(manifest);
        require(compiled.success,"compile four-family renderer fixture");
        const auto binary=manifest.parent_path()/"renderer.flxc";require(CompiledProjectWriter::write(binary.string(),compiled.project,d),"write renderer fixture");
        const auto loaded=CompiledProjectReader::read(binary.string());require(loaded.success,"read renderer fixture");
        Image frames[2]{};int index=0;
        for(const auto* project:{&compiled.project,&loaded.project})
        {
            const auto& definition=*project->resources.findObject(project->rootId);
            auto instance=RuntimeObjectBuilder::build(definition,"fixture","");
            const auto appearance=AnimationBaseAppearance::capture(instance.representation,instance.visualColor,instance.hasVisualColor);
            require(instance.animation.play("pose",definition.animations,appearance,d),"start fixture via internal C++ interface");
            frames[index++]=raster(instance,d,cache);
        }
        require(imagesEqual(frames[0],frames[1])&&ink(frames[0])>0,"source and compiled mixed-family animated pixels agree");
        UnloadImage(frames[0]);UnloadImage(frames[1]);
        const auto demo=compile(std::filesystem::path(FLX_SOURCE_DIR)/"microexamples/animation/persist-order/persist-order.flx");
        require(demo.success,"compile executable persist microexample");
        require(CompiledProjectWriter::write(binary.string(),demo.project,d),"write persist microexample");
        const auto executable=CompiledProjectReader::read(binary.string());require(executable.success,"read persist microexample");
        index=0;
        for(const auto* project:{&demo.project,&executable.project})
        {
            FlxSession session;FlxSessionOptions options;options.enableAudio=false;options.enableVideo=true;
            require(session.load(*project,options)&&session.render(),"execute persist example through production session");
            const auto output=session.output();frames[index]=LoadImageFromTexture(output);ImageFlipVertical(&frames[index]);
            require(equal(GetImageColor(frames[index],output.width/2,output.height/2),GREEN)&&
                equal(GetImageColor(frames[index],output.width*180/320,output.height/2),RED)&&
                equal(GetImageColor(frames[index],output.width*200/320,output.height/2),BLUE),"persist microexample has observable green/red/blue regions");
            ++index;session.unload();
        }
        require(imagesEqual(frames[0],frames[1]),"persist microexample open and compiled pixels agree");
        UnloadImage(frames[0]);UnloadImage(frames[1]);
        require(!d.hasErrors(),"graphics succeeds without fatal diagnostics");
    }
    if(own)CloseWindow();
}

