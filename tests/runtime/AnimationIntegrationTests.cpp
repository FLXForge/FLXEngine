#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace flx::test;

namespace
{
    constexpr float ScreenWidth = 320.0f;
    constexpr float ScreenHeight = 180.0f;
    constexpr float FrameDelta = 0.25f;

    struct RuntimeHarness
    {
        CompiledProject project;
        RuntimeWorld world;
        ScriptEngine scripts;

        RuntimeHarness()
        {
            project.rootId = "root";
            project.context.machine.video.screenWidth =
                static_cast<int>(ScreenWidth);
            project.context.machine.video.screenHeight =
                static_cast<int>(ScreenHeight);
            project.context.machine.video.outputScale = 1;
            scripts.setScreenScale(1);

            scripts.setFindObjectFunction(
                [this](const std::string& name)
                {
                    return world.findByName(name);
                }
            );

            scripts.setFindObjectByIdFunction(
                [this](const std::string& runtimeId)
                {
                    return world.findByRuntimeId(runtimeId);
                }
            );

            scripts.setSpawnObjectFunction(
                [this](RuntimeObject& source, const std::string& resourceId)
                {
                    world.spawn(source, resourceId, scripts);
                }
            );

            scripts.setKeepOnlyFunction(
                [this](const std::string& runtimeId)
                {
                    world.keepOnly(runtimeId);
                }
            );

            scripts.setKillObjectFunction(
                [this](const std::string& runtimeId)
                {
                    world.kill(runtimeId);
                }
            );

            scripts.setShowObjectFunction(
                [this](const std::string& runtimeId)
                {
                    world.show(runtimeId);
                }
            );

            scripts.setHideObjectFunction(
                [this](const std::string& runtimeId)
                {
                    world.hide(runtimeId);
                }
            );

            scripts.setRayCastFunction(
                [this](RuntimeObject& source, ScriptEngine& scriptEngine, float angle, float distance)
                {
                    return world.rayCast(source, scriptEngine, angle, distance);
                }
            );
        }

        void addScript(const std::string& id, const std::string& code)
        {
            ScriptResource script;
            script.id = id;
            script.sourceName = id + ".js";
            script.code = code;

            require(
                project.resources.addScript(id, script),
                "script should be added: " + id
            );
        }

        void addObject(ObjectDefinition definition)
        {
            if (definition.sourcePath.empty())
            {
                definition.sourcePath = definition.id + ".json";
            }

            require(
                project.resources.addObject(definition.id, definition),
                "object should be added: " + definition.id
            );
        }

        RuntimeLoadResult load()
        {
            return world.load(project, scripts);
        }

        void update(float delta = FrameDelta)
        {
            scripts.setFrameDelta(delta);
            world.update(scripts, delta);
        }
    };

    ObjectDefinition objectDefinition(
        const std::string& id,
        const std::string& script = ""
    )
    {
        ObjectDefinition definition;
        definition.id = id;
        definition.visible = true;

        if (!script.empty())
        {
            definition.resolvedScriptPaths.push_back(script);
        }

        return definition;
    }

    RuntimeObject& requireObject(
        RuntimeWorld& world,
        const std::string& name
    )
    {
        RuntimeObject* object =
            world.findByName(name);

        require(object != nullptr, "runtime object should exist: " + name);

        return *object;
    }

    double localValue(
        RuntimeObject& object,
        const std::string& key
    )
    {
        const auto it =
            object.local.find(key);

        if (it == object.local.end())
        {
            return 0.0;
        }

        if (const auto* number = std::get_if<double>(&it->second))
        {
            return *number;
        }

        if (const auto* boolean = std::get_if<bool>(&it->second))
        {
            return *boolean ? 1.0 : 0.0;
        }

        return 0.0;
    }

    bool nearlyEqual(
        double left,
        double right,
        double epsilon = 0.0001
    )
    {
        return std::abs(left - right) <= epsilon;
    }

    ObjectDefinition animated(const std::string& id = "root", const std::string& script = "probe")
    {
        auto d = objectDefinition(id, script);
        d.hasAnimations = true;
        AnimationDefinition a; a.duration = 1;
        AnimationKeyDefinition k; k.at = 1; k.transform.emplace(); k.transform->offsetX = 20;
        a.keys = {k}; d.animations["move"] = a;
        a.from = AnimationFrom::Current; a.hasFrom = true; d.animations["current"] = a;
        a.loop = true; a.hasLoop = true; d.animations["loop"] = a;
        a = {}; a.duration = 1; k = {}; k.at = 0; k.color = RED; a.keys = {k};
        d.animations["red"] = a;
        return d;
    }

    void check(RuntimeHarness& h, const std::string& body)
    {
        static int index = 0;
        const auto id = "assertion" + std::to_string(++index);
        h.scripts.loadScript(id, "function action(o) {" + body + "; write_local(o, 'checked', 1); }");
        auto& o = requireObject(h.world, "root"); o.local.erase("checked");
        h.scripts.callScriptFunction(id, "action", o);
        require(localValue(o, "checked") == 1, "JS assertions must complete: " + body);
    }

    void testPublicTransitions()
    {
        RuntimeHarness h;
        h.addScript("probe", R"(function born(o) {
            if (animation_current(o)!==null || animation_active(o) || animation_paused(o) || animation_done(o) || animation_left(o)!==0) throw Error('absent');
            pause_animation(o); stop_animation(o); play_animation(o, 'move'); play_timer(o, 'control', 1);
            if (animation_current(o)!=='move' || !animation_active(o) || animation_paused(o) || animation_done(o) || animation_left(o)!==1) throw Error('running');
            write_local(o, 'bornLeft', animation_left(o));
        })");
        h.addObject(animated()); require(h.load().success, "load animation JS");
        auto& o = requireObject(h.world,"root"); require(localValue(o,"bornLeft")==1,"born begins without elapsed time");
        h.update(); require(nearlyEqual(o.animation.left(),.75),"one time advance");
        check(h,"play_animation(o,'move'); if(animation_left(o)!==0.75) throw Error('running restart'); pause_animation(o); pause_animation(o); if(!animation_active(o)||!animation_paused(o)||animation_done(o)) throw Error('pause')");
        h.update(); require(nearlyEqual(o.animation.left(),.75),"pause freezes animation");
        require(nearlyEqual(o.timers.at("control").left,.5),"Timer remains independent");
        check(h,"play_animation(o,'move'); if(animation_paused(o)||animation_left(o)!==0.75) throw Error('resume')");
        h.update(.75f); require(o.animation.done(),"complete");
        check(h,"if(animation_active(o)||animation_paused(o)||!animation_done(o)||animation_left(o)!==0||animation_current(o)!=='move') throw Error('done'); pause_animation(o); if(!animation_done(o)) throw Error('done pause')");
        auto base = AnimationBaseAppearance::capture(o.representation);
        require(o.animation.appearance(base).transform.offsetX==20,"DONE holds final appearance");
        check(h,"play_animation(o,'move'); if(animation_left(o)!==1||!animation_active(o)) throw Error('restart')");
        h.update();
        check(h,"play_animation(o,'current')");
        require(o.animation.reference().transform.offsetX==5,"replacement captures current only controlled components");
        h.update(); require(o.animation.appearance(base).transform.offsetX==10,"current adds to capture");
        check(h,"stop_animation(o); stop_animation(o); if(animation_current(o)!==null||animation_active(o)||animation_done(o)||animation_left(o)!==0) throw Error('stop')");
        require(o.animation.appearance(base).transform.offsetX==0,"stop removes layer");
        check(h,"play_animation(o,'loop')"); h.update(2.5f);
        check(h,"if(animation_left(o)!==0.5||animation_done(o)||!animation_active(o)) throw Error('loop')");
        require(o.animation.appearance(base).transform.offsetX==50,"loop current multi cycle");
        check(h,"pause_animation(o); stop_animation(o); if(animation_paused(o)) throw Error('paused stop')");
        check(h,"play_animation(o,'move')"); h.update(1); check(h,"stop_animation(o); if(animation_done(o)) throw Error('done stop')");
    }

    void testInvalidAndExpiredViews()
    {
        RuntimeHarness h;
        h.addScript("probe", R"(let stale;
          function born(o) { stale=o; play_animation(o,'move'); }
          function action(o) {
            play_animation(stale,'red'); pause_animation(stale); stop_animation(stale);
            if(animation_current(stale)!==null || animation_active(stale)||animation_paused(stale)||animation_done(stale)||animation_left(stale)!==0) throw Error('stale defaults');
            for(const v of [null,undefined,0,false,'root',{}]) {
                play_animation(v,'red'); pause_animation(v); stop_animation(v);
                if(animation_current(v)!==null||animation_active(v)||animation_paused(v)||animation_done(v)||animation_left(v)!==0) throw Error('invalid object');
            }
            play_animation(); pause_animation(); stop_animation();
            animation_current(); animation_active(); animation_paused(); animation_done(); animation_left();
            for(const name of [undefined,null,false,2,{},'', 'missing', 'move\u0000suffix']) play_animation(o,name);
            if(animation_current(o)!=='move'||animation_left(o)!==1) throw Error('invalid changed playback');
            write_local(o,'continued',1);
          })");
        h.addObject(animated()); StreamCapture capture;
        require(h.load().success,"load invalid probe"); h.update();
        auto& o=requireObject(h.world,"root"); require(localValue(o,"continued")==1,"invalid calls continue callback");
        require(o.animation.left()==.75,"invalid calls preserve clock");
        require((capture.output.str()+capture.error.str()).find("FLX-RUNTIME-00020")!=std::string::npos,"invalid diagnostics emitted");
    }

    void testLifecycleAndIsolation()
    {
        for(const auto& hook : {"born","action","motion"}) {
            RuntimeHarness h; h.addScript("probe",std::string("function ")+hook+"(o) {play_animation(o,'move');write_local(o,'inside',animation_left(o));}");
            h.addObject(animated()); require(h.load().success,"load hook"); h.update();
            auto& o=requireObject(h.world,"root"); require(o.animation.left()==.75,"same update temporal phase: "+std::string(hook));
            require(localValue(o,"inside")==1,"hook sees initial time");
        }
        RuntimeHarness h;
        h.addScript("probe", "function born(o){play_animation(o,'move');hide(o);}");
        auto root=animated(); root.childResources["child"]="child"; h.addObject(root);
        auto child=animated("child",""); h.addObject(child);
        require(h.load().success,"load hidden and independent instances"); h.update(.5);
        auto& o=requireObject(h.world,"root"); auto& c=requireObject(h.world,"child");
        require(!o.visible&&o.animation.left()==.5,"hidden objects advance");
        require(!c.animation.current(),"child independent");
        check(h,"show(o); if(animation_left(o)!==0.5) throw Error('show reset')");
        h.addScript("deadProbe", "function dead(o){write_global('deadLeft',animation_left(o));write_global('deadCurrent',animation_current(o));}");
        h.scripts.loadScript("deadProbe",h.project.resources.findScript("deadProbe")->code);
        o.scripts.push_back("deadProbe"); o.resolvedScriptPaths.push_back("deadProbe");
        const auto id=o.runtimeId;
        h.world.kill(id); h.update(.25);
        auto value=h.scripts.readGlobalValue("deadLeft"); require(value&&std::get<double>(*value)==.5,"dead hook queries last unadvanced time");
        require(h.world.findByRuntimeId(id)==nullptr,"destroyed object removed");
    }

    void testSpawnAndColor()
    {
        RuntimeHarness h;
        h.addScript("probe","function action(o){if(!read_local(o,'spawned')){spawn(o,'child');write_local(o,'spawned',1);}} ");
        h.addScript("childProbe","function born(o){play_animation(o,'move');}");
        auto root=animated(); root.childResources["child"]="child"; h.addObject(root);
        auto child=animated("child","childProbe"); child.spawnMode="manual"; h.addObject(child);
        require(h.load().success,"load spawn"); h.update();
        require(requireObject(h.world,"child").animation.left()==.75,"spawn before temporal phase advances same update");
        auto& o=requireObject(h.world,"root"); const auto position=o.position;const auto size=o.size;const auto angle=o.angle;const auto depth=o.depth;
        check(h,"apply_color(o,'blue');play_animation(o,'red');apply_color(o,'green')");
        auto base=[&](){return AnimationBaseAppearance::capture(o.representation,o.hasVisualColor?o.visualColor:WHITE,o.hasVisualColor);};
        require(o.animation.appearance(base()).color.r==RED.r&&o.animation.appearance(base()).color.g==RED.g,"animation overrides live base color");
        check(h,"pause_animation(o);restore_color(o)");require(o.animation.appearance(base()).color.r==RED.r,"paused color retained");
        check(h,"play_animation(o,'red')");h.update(1);
        check(h,"apply_color(o,'blue');if(!animation_done(o)) throw Error('color done')");
        require(o.animation.appearance(base()).color.r==RED.r,"DONE color overrides changed live base");
        check(h,"restore_color(o);stop_animation(o)");require(!o.hasVisualColor,"stop preserves runtime color absence");
        require(o.position.x==position.x&&o.position.y==position.y&&o.size.x==size.x&&o.size.y==size.y&&o.angle==angle&&o.depth==depth,"spatial state unaffected");
    }
    void testCollisionAndState()
    {
        RuntimeHarness h;
        h.addScript("probe",R"(function collision(o,other){
            play_animation(o,'move');state_to(o,'next');
            write_local(o,'inside',animation_left(o));
        })");
        auto root=animated();root.origin={20,20};root.hasOrigin=true;root.size={10,10};root.hasSize=true;
        root.initialState="initial";root.stateTransitions["initial"]={"next"};root.stateTransitions["next"]={};
        root.collisions["body"].with={"target"};root.childResources["target"]="target";
        auto target=objectDefinition("target");target.group="target";target.offset={0,0};target.hasOffset=true;
        target.size={100,100};target.hasSize=true;target.collisions["body"];
        h.addObject(root);h.addObject(target);require(h.load().success,"collision/state load");h.update();
        auto& o=requireObject(h.world,"root");
        require(localValue(o,"inside")==1&&o.animation.left()==.75,"collision starts before temporal phase");
        require(o.state=="next","state transition executed");
        check(h,"state_to(o,'next');if(animation_left(o)!==0.75) throw Error('state reset')");
        h.update();require(o.animation.left()==.5,"state does not reset playback");
    }

    void testPostTemporalCreation()
    {
        RuntimeHarness h;
        h.addScript("probe","");h.addScript("childProbe","function born(o){play_animation(o,'move');}");
        auto root=objectDefinition("root");root.creationMode="iterator";root.iteratorPattern={"first","second"};
        root.childResources["first"]="first";root.childResources["second"]="second";
        h.addObject(root);h.addObject(animated("first","childProbe"));h.addObject(animated("second","childProbe"));
        require(h.load().success,"load iterator animations");
        require(requireObject(h.world,"first").animation.left()==1,"load born has no time advance");
        const auto id=requireObject(h.world,"first").runtimeId;h.world.kill(id);h.update();
        require(requireObject(h.world,"second").animation.left()==1,"iterator born after temporal phase keeps time zero");
        h.update();require(requireObject(h.world,"second").animation.left()==.75,"iterator advances in subsequent update");
    }

    void testMicroexamplesOpenCompiled()
    {
        for(const std::string name:{"bitmap-invader","interpolation","loop-base","loop-current","pause-resume","representation","color","persist-order"}) {
            const auto path=std::filesystem::path(FLX_SOURCE_DIR)/"microexamples/animation"/name/(name+".flx");
            const auto compiled=compile(path);require(compiled.success,"compile microexample "+name);
            Diagnostics diagnostics;const auto binary=testRoot()/("animation-"+name+".flxc");
            require(CompiledProjectWriter::write(binary.string(),compiled.project,diagnostics),"write microexample "+name);
            const auto decoded=CompiledProjectReader::read(binary.string());require(decoded.success,"read microexample "+name);
            std::vector<double> traces[2];int index=0;
            for(const auto* project:{&compiled.project,&decoded.project}) {
                RuntimeHarness h;h.project=*project;require(h.load().success,"execute microexample "+name);
                if(name=="persist-order") {++index;continue;}
                for(int frame=0;frame<=24;++frame) {
                    auto& o=requireObject(h.world,"sample");const auto appearance=o.animation.appearance(AnimationBaseAppearance::capture(o.representation,o.visualColor,o.hasVisualColor));
                    traces[index].insert(traces[index].end(),{o.animation.left(),appearance.transform.offsetX,appearance.transform.rotation,double(appearance.color.r),double(appearance.replaceable->size()),double(o.animation.paused())});
                    if(name=="bitmap-invader") {
                        const auto* definition=project->resources.findObject(o.definitionId);
                        require(definition,"bitmap invader definition");
                        const auto& keys=definition->animations.at("march").keys;
                        require(keys.size()==5&&appearance.replaceable->size()==1,"five bitmap key frames");
                        require(appearance.replaceable->at(0).bitmap==keys[frame%5].representation->at(0).bitmap,"exact discrete frame at each boundary including loop restart");
                        require(o.animation.active()&&!o.animation.done(),"bitmap animation loops without DONE");
                    }
                    if(frame==2&&name=="interpolation") require(appearance.transform.offsetX==20,"JS interpolation visible offset");
                    if(frame==12&&name=="loop-current") require(appearance.transform.offsetX==60,"current cumulative example");
                    if(frame==12&&name=="loop-base") require(appearance.transform.offsetX==30,"base reset example");
                    if(frame==6&&name=="pause-resume") require(o.animation.paused()&&o.animation.left()==3,"pause example actually pauses");
                    if(frame==10&&name=="pause-resume") require(!o.animation.paused()&&o.animation.left()==2.5,"resume example resumes");
                    if(frame==8&&name=="representation") require(appearance.replaceable->empty(),"empty replacement example");
                    h.update(.25);
                }
                ++index;
            }
            require(traces[0]==traces[1],"source/compiled execution equivalence "+name);
        }
    }
    void testMachinePaletteNoReprojection()
    {
        const auto manifest=createMinimalProject("animation_audit_machine");
        writeFile(manifest,"name=AnimationAudit\npath=game\nroot=root\nmachine=machine.yml\n");
        writeFile(manifest.parent_path()/"machine.yml","machine:\n  video:\n    color:\n      palette: ['#ff0000', '#0000ff']\n");
        writeFile(manifest.parent_path()/"game/root.json",R"({
          "visual":{"color":"#ff0000","representation":[{"primitive":"rectangle"}]},
          "behavior":{"scripts":["start"]},
          "animations":{
            "shade":{"duration":1,"keys":[{"at":1,"color":"#0000ff"}]},
            "next":{"duration":1,"from":"current","keys":[{"at":1,"color":"#ff0000"}]},
            "overflow":{"duration":1,"loop":true,"from":"current","keys":[{"at":0,"color":"#ff0000"},{"at":1,"transform":{"offset":{"x":1e308}},"color":"#0000ff"}]}
          }
        })");
        writeFile(manifest.parent_path()/"game/start.js","function born(o){play_animation(o,'shade');}");
        const auto source=compile(manifest);require(source.success,"compile discrete Machine palette");
        Diagnostics diagnostics;const auto binary=manifest.parent_path()/"machine.flxc";
        require(CompiledProjectWriter::write(binary.string(),source.project,diagnostics),"write Machine Animation");
        const auto loaded=CompiledProjectReader::read(binary.string());require(loaded.success,"read Machine Animation");
        for(const auto* project:{&source.project,&loaded.project}) {
            RuntimeHarness h;h.project=*project;require(h.load().success,"load Machine Animation runtime");h.update(.5);
            const auto& o=requireObject(h.world,"root");
            const auto appearance=o.animation.appearance(AnimationBaseAppearance::capture(o.representation,o.visualColor,o.hasVisualColor));
            require(appearance.color.r==128&&appearance.color.g==0&&appearance.color.b==128&&appearance.color.a==255,
                "Machine effective endpoints interpolate outside discrete palette without reprojection");
            check(h,"play_animation(o,'next');apply_color(o,'#0000ff')");h.update(.5);
            const auto base=AnimationBaseAppearance::capture(o.representation,o.visualColor,o.hasVisualColor);
            const auto captured=o.animation.appearance(base);
            require(captured.color.r==192&&captured.color.b==64,"current color reference remains captured after apply_color, no Machine reprojection");
            check(h,"stop_animation(o);play_animation(o,'overflow')");h.update(.5);
            const auto valid=o.animation.appearance(base);
            StreamCapture capture;
            h.update(2);
            require(o.animation.elapsed()==2.5&&o.animation.left()==.5,"CPU failure preserves representable World clock");
            const auto retained=o.animation.appearance(base);
            require(retained.transform.offsetX==valid.transform.offsetX&&retained.color.r==valid.color.r&&retained.color.b==valid.color.b,"CPU fallback is atomic in source and compiled World");
            check(h,"pause_animation(o);play_animation(o,'overflow')");
            for(int frame=0;frame<8;++frame)h.update(1);
            require(countOccurrences(capture.output.str()+capture.error.str(),"FLX-RUNTIME-00021")==1,"World CPU recovery warns once, including resume");
            check(h,"stop_animation(o);play_animation(o,'overflow')");h.update(3);
            require(countOccurrences(capture.output.str()+capture.error.str(),"FLX-RUNTIME-00021")==2,"new playback owns its diagnostic lifetime");
        }
    }

}

int main()
{
    const std::vector<std::pair<std::string,void(*)()>> tests={
        {"public transitions and queries",testPublicTransitions},
        {"invalid arguments and expired views",testInvalidAndExpiredViews},
        {"lifecycle hidden dead and independence",testLifecycleAndIsolation},
        {"spawn color and spatial invariants",testSpawnAndColor},
        {"collision and State",testCollisionAndState},
        {"post-temporal iterator creation",testPostTemporalCreation},
        {"microexamples open and compiled",testMicroexamplesOpenCompiled},
        {"Machine palette midpoint source and compiled",testMachinePaletteNoReprojection}
    };
    int failures=0;
    for(const auto& [name,test]:tests)try{test();std::cout<<"[PASS] "<<name<<'\n';}catch(const std::exception& e){++failures;std::cerr<<"[FAIL] "<<name<<": "<<e.what()<<'\n';}
    return failures?1:0;
}
