#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    constexpr float ScreenWidth = 320.0f;
    constexpr float ScreenHeight = 180.0f;
    constexpr float FrameDelta = 0.1f;

    struct RuntimeHarness
    {
        CompiledProject project;
        RuntimeWorld world;
        ScriptEngine scripts;

        RuntimeHarness()
        {
            project.rootId = "root";
            project.context.machine.video.screenWidth = static_cast<int>(ScreenWidth);
            project.context.machine.video.screenHeight = static_cast<int>(ScreenHeight);
            project.context.machine.video.outputScale = 1;

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
                [this](RuntimeObject& source, float angle, float distance)
                {
                    return world.rayCast(source, angle, distance);
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
            world.update(scripts, ScreenWidth, ScreenHeight, delta);
        }

        void draw()
        {
            world.draw(scripts, 1, ScreenWidth, ScreenHeight, false);
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
        definition.shapeType = "none";

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

        return it->second;
    }

    bool nearlyEqual(
        double left,
        double right,
        double epsilon = 0.0001
    )
    {
        return std::abs(left - right) <= epsilon;
    }

    class HiddenTestWindow
    {
    public:
        HiddenTestWindow()
        {
            if (!IsWindowReady())
            {
                SetTraceLogLevel(LOG_WARNING);
                SetConfigFlags(FLAG_WINDOW_HIDDEN);
                InitWindow(1, 1, "FLX runtime lifecycle test");
                opened = true;
            }
        }

        ~HiddenTestWindow()
        {
            if (opened)
            {
                CloseWindow();
            }
        }

    private:
        bool opened = false;
    };

    void testInitialLoadBornOrderAndIdentity()
    {
        RuntimeHarness harness;

        harness.addScript(
            "recordBorn",
            "function born(o) {"
            "  if (global['order'] == null) global['order'] = 0;"
            "  if (o.local['bornCount'] == null) o.local['bornCount'] = 0;"
            "  o.local['bornOrder'] = global['order'];"
            "  o.local['bornCount'] = o.local['bornCount'] + 1;"
            "  global['order'] = global['order'] + 1;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "recordBorn");
        root.childResources["parent"] = "parent";

        ObjectDefinition parent =
            objectDefinition("parent", "recordBorn");
        parent.childResources["child"] = "child";

        ObjectDefinition child =
            objectDefinition("child", "recordBorn");

        harness.addObject(root);
        harness.addObject(parent);
        harness.addObject(child);

        RuntimeLoadResult result =
            harness.load();

        require(result.success, "runtime should load initial graph");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeParent =
            requireObject(harness.world, "parent");
        RuntimeObject& runtimeChild =
            requireObject(harness.world, "child");

        require(localValue(runtimeRoot, "bornCount") == 1.0, "root born should run once");
        require(localValue(runtimeParent, "bornCount") == 1.0, "parent born should run once");
        require(localValue(runtimeChild, "bornCount") == 1.0, "child born should run once");

        require(localValue(runtimeRoot, "bornOrder") == 0.0, "root born order should be first");
        require(localValue(runtimeParent, "bornOrder") == 1.0, "parent born order should follow root");
        require(localValue(runtimeChild, "bornOrder") == 2.0, "child born order should follow parent");

        require(runtimeRoot.runtimeId != runtimeParent.runtimeId, "root and parent ids should differ");
        require(runtimeParent.runtimeId != runtimeChild.runtimeId, "parent and child ids should differ");
        require(runtimeChild.parentId == runtimeParent.runtimeId, "child parentId should point to parent");
        require(runtimeChild.originalParentId == runtimeParent.runtimeId, "child originalParentId should point to parent");
    }

    void testSpawnDuringBornIsStabilizedBeforeLoadReturns()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnerBorn",
            "function born(o) { spawn(o, 'manual'); }"
            "function action(o) { o.local['actionCount'] = (o.local['actionCount'] || 0) + 1; }"
        );

        harness.addScript(
            "spawnedProbe",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; }"
            "function action(o) { o.local['actionCount'] = (o.local['actionCount'] || 0) + 1; }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnerBorn");
        root.childResources["manual"] = "spawned";

        ObjectDefinition spawned =
            objectDefinition("spawned", "spawnedProbe");
        spawned.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(spawned);

        require(harness.load().success, "runtime should load born spawn project");

        RuntimeObject& runtimeSpawned =
            requireObject(harness.world, "spawned");

        require(localValue(runtimeSpawned, "bornCount") == 1.0, "born-spawned object should be born once");
        require(localValue(runtimeSpawned, "actionCount") == 0.0, "born-spawned object should not run action during load");

        harness.update();

        RuntimeObject& updatedSpawned =
            requireObject(harness.world, "spawned");

        require(localValue(updatedSpawned, "actionCount") == 1.0, "born-spawned object should join first frame action");
        require(localValue(updatedSpawned, "motionCount") == 1.0, "born-spawned object should join first frame motion");
    }

    void testRecursiveSpawnDuringBornIsStabilizedBeforeLoadReturns()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnA",
            "function born(o) { spawn(o, 'a'); }"
        );

        harness.addScript(
            "spawnB",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; spawn(o, 'b'); }"
        );

        harness.addScript(
            "bornProbe",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnA");
        root.childResources["a"] = "a";

        ObjectDefinition a =
            objectDefinition("a", "spawnB");
        a.spawnMode = "manual";
        a.childResources["b"] = "b";

        ObjectDefinition b =
            objectDefinition("b", "bornProbe");
        b.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(a);
        harness.addObject(b);

        require(harness.load().success, "runtime should stabilize recursive born spawns");

        RuntimeObject& runtimeA =
            requireObject(harness.world, "a");
        RuntimeObject& runtimeB =
            requireObject(harness.world, "b");

        require(localValue(runtimeA, "bornCount") == 1.0, "recursive spawned parent should be born once");
        require(localValue(runtimeB, "bornCount") == 1.0, "recursive spawned child should be born once");
    }

    void testSpawnDuringBornLimitFailsLoad()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnSelf",
            "function born(o) { spawn(o, 'self'); }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnSelf");
        root.childResources["self"] = "root";
        root.spawnMode = "manual";

        harness.addObject(root);

        RuntimeLoadResult result =
            harness.load();

        require(!result.success, "runtime load should fail when born spawn never stabilizes");
        require(result.diagnostics.hasErrors(), "born spawn limit should produce diagnostics");
    }

    void testSpawnDuringActionMotionAndCollisionPhases()
    {
        RuntimeHarness harness;

        harness.addScript(
            "phaseSpawner",
            "function action(o) { if (o.local['a'] == null) { o.local['a'] = 1; spawn(o, 'actionChild'); } }"
            "function motion(o) { if (o.local['m'] == null) { o.local['m'] = 1; spawn(o, 'motionChild'); } }"
            "function collision(o, other) { if (o.local['c'] == null) { o.local['c'] = 1; spawn(o, 'collisionChild'); } }"
        );

        harness.addScript(
            "phaseProbe",
            "function born(o) { o.local['bornCount'] = (o.local['bornCount'] || 0) + 1; }"
            "function action(o) { o.local['actionCount'] = (o.local['actionCount'] || 0) + 1; }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "phaseSpawner");
        root.childResources["actionChild"] = "actionChild";
        root.childResources["motionChild"] = "motionChild";
        root.childResources["collisionChild"] = "collisionChild";
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "circle";
        root.collisionWith.push_back("target");
        root.group = "source";
        root.origin = Vector2{ 20.0f, 20.0f };
        root.hasOrigin = true;
        root.size = Vector2{ 10.0f, 10.0f };
        root.collisionRadius = 8.0f;

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "circle";
        target.offset = Vector2{ 0.0f, 0.0f };
        target.hasOffset = true;
        target.size = Vector2{ 10.0f, 10.0f };
        target.collisionRadius = 8.0f;

        ObjectDefinition actionChild =
            objectDefinition("actionChild", "phaseProbe");
        actionChild.spawnMode = "manual";
        ObjectDefinition motionChild =
            objectDefinition("motionChild", "phaseProbe");
        motionChild.spawnMode = "manual";
        ObjectDefinition collisionChild =
            objectDefinition("collisionChild", "phaseProbe");
        collisionChild.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(target);
        harness.addObject(actionChild);
        harness.addObject(motionChild);
        harness.addObject(collisionChild);

        require(harness.load().success, "runtime should load phase spawn project");
        RuntimeObject& loadedRoot =
            requireObject(harness.world, "root");
        RuntimeObject& loadedTarget =
            requireObject(harness.world, "target");
        require(loadedRoot.collisionActive, "root collision should be active");
        require(loadedRoot.collisionType == "circle", "root collision should be circle");
        require(loadedTarget.collisionType == "circle", "target collision should be circle");
        require(loadedTarget.group == "target", "target group should be target");
        require(loadedRoot.position.x == loadedTarget.position.x, "root and target x should match");
        require(loadedRoot.position.y == loadedTarget.position.y, "root and target y should match");

        harness.update();

        RuntimeObject& updatedRoot =
            requireObject(harness.world, "root");
        require(
            localValue(updatedRoot, "c") == 1.0,
            "collision callback should run"
        );

        RuntimeObject& actionRuntime =
            requireObject(harness.world, "actionChild");
        RuntimeObject& motionRuntime =
            requireObject(harness.world, "motionChild");
        RuntimeObject& collisionRuntime =
            requireObject(harness.world, "collisionChild");

        require(localValue(actionRuntime, "bornCount") == 1.0, "action spawn should be born once");
        require(localValue(actionRuntime, "actionCount") == 0.0, "action spawn should miss current action phase");
        require(localValue(actionRuntime, "motionCount") == 1.0, "action spawn should join current motion phase");

        require(localValue(motionRuntime, "bornCount") == 1.0, "motion spawn should be born once");
        require(localValue(motionRuntime, "actionCount") == 0.0, "motion spawn should miss current action phase");
        require(localValue(motionRuntime, "motionCount") == 0.0, "motion spawn should miss current motion phase");

        require(localValue(collisionRuntime, "bornCount") == 1.0, "collision spawn should be born once");
        require(localValue(collisionRuntime, "actionCount") == 0.0, "collision spawn should miss current action phase");
        require(localValue(collisionRuntime, "motionCount") == 0.0, "collision spawn should miss current motion phase");
    }

    void testKillDuringActionMotionAndCollisionIsTerminal()
    {
        RuntimeHarness harness;

        harness.addScript(
            "killInAction",
            "function action(o) { kill(o); }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
            "function dead(o) { o.local['deadCount'] = (o.local['deadCount'] || 0) + 1; o.alive = true; }"
        );

        harness.addScript(
            "killInMotion",
            "function motion(o) { kill(o); }"
            "function collision(o, other) { o.local['collisionCount'] = (o.local['collisionCount'] || 0) + 1; }"
            "function dead(o) { o.local['deadCount'] = (o.local['deadCount'] || 0) + 1; o.alive = true; }"
        );

        harness.addScript(
            "killInCollision",
            "function collision(o, other) { kill(o); }"
            "function dead(o) { o.local['deadCount'] = (o.local['deadCount'] || 0) + 1; o.alive = true; }"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["actionVictim"] = "actionVictim";
        root.childResources["motionVictim"] = "motionVictim";
        root.childResources["collisionVictim"] = "collisionVictim";
        root.childResources["target"] = "target";

        ObjectDefinition actionVictim =
            objectDefinition("actionVictim", "killInAction");

        ObjectDefinition motionVictim =
            objectDefinition("motionVictim", "killInMotion");
        motionVictim.collisionActive = true;
        motionVictim.collisionType = "box";
        motionVictim.collisionWith.push_back("target");
        motionVictim.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition collisionVictim =
            objectDefinition("collisionVictim", "killInCollision");
        collisionVictim.collisionActive = true;
        collisionVictim.collisionType = "box";
        collisionVictim.collisionWith.push_back("target");
        collisionVictim.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(actionVictim);
        harness.addObject(motionVictim);
        harness.addObject(collisionVictim);
        harness.addObject(target);

        require(harness.load().success, "runtime should load kill project");

        harness.update();

        require(harness.world.findByName("actionVictim") == nullptr, "kill in action should be terminal");
        require(harness.world.findByName("motionVictim") == nullptr, "kill in motion should be terminal");
        require(harness.world.findByName("collisionVictim") == nullptr, "kill in collision should be terminal");
    }

    void testAliveIsReadOnlyFromJavaScript()
    {
        RuntimeHarness harness;

        harness.addScript(
            "writeAlive",
            "function action(o) { o.alive = false; }"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "writeAlive");

        harness.addObject(root);

        require(harness.load().success, "runtime should load alive readonly project");

        harness.update();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(runtimeRoot.alive, "direct JS alive write should not kill object");
        require(localValue(runtimeRoot, "motionCount") == 1.0, "object should keep participating after ignored alive write");
    }

    void testKeepOnlyPreventsOtherObjectsFromResurrecting()
    {
        RuntimeHarness harness;

        harness.addScript(
            "keepOnlyRoot",
            "function action(o) { if (o.local['done'] == null) { o.local['done'] = 1; keep_only(o); } }"
        );

        harness.addScript(
            "resurrectInDead",
            "function dead(o) { o.local['deadCount'] = (o.local['deadCount'] || 0) + 1; o.alive = true; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "keepOnlyRoot");
        root.childResources["resurrect"] = "resurrect";

        ObjectDefinition resurrect =
            objectDefinition("resurrect", "resurrectInDead");

        harness.addObject(root);
        harness.addObject(resurrect);

        require(harness.load().success, "runtime should load keep_only project");

        const std::string rootId =
            requireObject(harness.world, "root").runtimeId;

        harness.update();

        require(harness.world.findByRuntimeId(rootId) != nullptr, "keep_only should keep only exact object");
        require(harness.world.findByName("resurrect") == nullptr, "keep_only should keep non-kept object dead even if dead sets alive=true");
    }

    void testHideSuppressesDrawButKeepsRuntimePhasesAndShowRestoresDraw()
    {
        RuntimeHarness harness;

        harness.addScript(
            "visibilityProbe",
            "function born(o) { timer(o, 'life', 1.0); }"
            "function action(o) {"
            "  o.local['actionCount'] = (o.local['actionCount'] || 0) + 1;"
            "  if (o.local['actionCount'] == 1) hide(o);"
            "  if (o.local['actionCount'] == 2) show(o);"
            "}"
            "function motion(o) { o.local['motionCount'] = (o.local['motionCount'] || 0) + 1; }"
            "function collision(o, other) { o.local['collisionCount'] = (o.local['collisionCount'] || 0) + 1; }"
            "function draw(o) { o.local['drawCount'] = (o.local['drawCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "visibilityProbe");
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "box";
        root.collisionWith.push_back("target");
        root.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(target);

        require(harness.load().success, "runtime should load visibility project");

        HiddenTestWindow window;

        harness.update(0.25f);
        harness.draw();

        RuntimeObject& hiddenRoot =
            requireObject(harness.world, "root");

        require(!hiddenRoot.visible, "hide should mark object invisible");
        require(localValue(hiddenRoot, "actionCount") == 1.0, "hidden object should run action");
        require(localValue(hiddenRoot, "motionCount") == 1.0, "hidden object should run motion");
        require(localValue(hiddenRoot, "collisionCount") == 1.0, "hidden object should still collide");
        require(localValue(hiddenRoot, "drawCount") == 0.0, "hide should suppress JS draw");
        require(nearlyEqual(hiddenRoot.timers["life"].left, 0.75), "hidden object timers should continue");

        harness.update(0.25f);
        harness.draw();

        RuntimeObject& shownRoot =
            requireObject(harness.world, "root");

        require(shownRoot.visible, "show should mark object visible");
        require(localValue(shownRoot, "drawCount") == 1.0, "show should restore JS draw");
    }

    void testVisibleIsReadOnlyFromJavaScript()
    {
        RuntimeHarness harness;

        harness.addScript(
            "writeVisible",
            "function action(o) { o.visible = false; }"
            "function draw(o) { o.local['drawCount'] = (o.local['drawCount'] || 0) + 1; }"
        );

        ObjectDefinition root =
            objectDefinition("root", "writeVisible");

        harness.addObject(root);

        require(harness.load().success, "runtime should load visible readonly project");

        HiddenTestWindow window;

        harness.update();
        harness.draw();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(runtimeRoot.visible, "direct JS visible write should be ignored");
        require(localValue(runtimeRoot, "drawCount") == 1.0, "object should still draw after ignored visible write");
    }

    void testDrawCallbacksFollowStableLayerOrder()
    {
        RuntimeHarness harness;

        harness.addScript(
            "drawProbe",
            "function draw(o) {"
            "  if (global['drawOrder'] == null) global['drawOrder'] = 0;"
            "  o.local['drawOrder'] = global['drawOrder'];"
            "  global['drawOrder'] = global['drawOrder'] + 1;"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "drawProbe");
        root.childResources["low"] = "low";
        root.childResources["same"] = "same";
        root.childResources["high"] = "high";

        ObjectDefinition low =
            objectDefinition("low", "drawProbe");
        low.layer = -10;

        ObjectDefinition same =
            objectDefinition("same", "drawProbe");
        same.layer = 0;

        ObjectDefinition high =
            objectDefinition("high", "drawProbe");
        high.layer = 10;

        harness.addObject(root);
        harness.addObject(low);
        harness.addObject(same);
        harness.addObject(high);

        require(harness.load().success, "runtime should load draw project");

        HiddenTestWindow window;

        harness.draw();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        RuntimeObject& runtimeLow =
            requireObject(harness.world, "low");
        RuntimeObject& runtimeSame =
            requireObject(harness.world, "same");
        RuntimeObject& runtimeHigh =
            requireObject(harness.world, "high");

        require(localValue(runtimeLow, "drawOrder") == 0.0, "lower layer should draw first");
        require(localValue(runtimeRoot, "drawOrder") < localValue(runtimeSame, "drawOrder"), "same layer should keep insertion order");
        require(localValue(runtimeHigh, "drawOrder") > localValue(runtimeSame, "drawOrder"), "higher layer should draw last");
    }

    void testObjectTimeStateAndTimers()
    {
        RuntimeHarness harness;

        harness.addScript(
            "timeProbe",
            "function born(o) { timer(o, 'life', 1.0); }"
            "function action(o) {"
            "  if (o.local['firstStateTime'] == null) o.local['firstStateTime'] = state_time(o);"
            "  if (o.local['firstTimerLeft'] == null) o.local['firstTimerLeft'] = timer_left(o, 'life');"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "timeProbe");
        root.initialState = "start";
        root.stateTransitions["start"].push_back("next");
        root.stateTransitions["next"];

        harness.addObject(root);

        require(harness.load().success, "runtime should load time project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(runtimeRoot.stateTime == 0.0f, "state time should start at zero");
        require(runtimeRoot.timers["life"].left == 1.0f, "timer should keep born value before first update");

        harness.update(0.25f);

        RuntimeObject& updatedRoot =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(updatedRoot, "firstStateTime"), 0.0), "action sees state time before frame increment");
        require(nearlyEqual(localValue(updatedRoot, "firstTimerLeft"), 1.0), "action sees timer before frame decrement");
        require(nearlyEqual(updatedRoot.stateTime, 0.25), "state time increments after collision phase");
        require(nearlyEqual(updatedRoot.timers["life"].left, 0.75), "timer decrements after collision phase");
    }

    void testAttachmentsApplyAfterMotionBeforeCollision()
    {
        RuntimeHarness harness;

        harness.addScript(
            "parentMotion",
            "function motion(o) { o.x = o.x + 10; o.y = o.y + 5; o.angle = o.angle + 15; }"
        );

        harness.addScript(
            "attachedChild",
            "function motion(o) { o.local['motionX'] = o.x; o.local['motionY'] = o.y; o.local['motionAngle'] = o.angle; }"
            "function collision(o, other) { o.local['collisionX'] = o.x; o.local['collisionY'] = o.y; o.local['collisionAngle'] = o.angle; }"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["parent"] = "parent";

        ObjectDefinition parent =
            objectDefinition("parent", "parentMotion");
        parent.childResources["child"] = "child";
        parent.origin = Vector2{ 30.0f, 40.0f };
        parent.hasOrigin = true;
        parent.angle = 20.0f;

        ObjectDefinition child =
            objectDefinition("child", "attachedChild");
        child.offset = Vector2{ 3.0f, 4.0f };
        child.hasOffset = true;
        child.attachOnCreate = true;
        child.attachFollowX = true;
        child.attachFollowY = false;
        child.attachFollowAngle = true;
        child.collisionActive = true;
        child.collisionType = "box";
        child.collisionWith.push_back("parent");
        child.size = Vector2{ 10.0f, 10.0f };
        child.group = "child";

        parent.group = "parent";
        parent.collisionType = "box";
        parent.size = Vector2{ 30.0f, 30.0f };

        harness.addObject(root);
        harness.addObject(parent);
        harness.addObject(child);

        require(harness.load().success, "runtime should load attachment project");

        harness.update();

        RuntimeObject& runtimeChild =
            requireObject(harness.world, "child");
        RuntimeObject& runtimeParent =
            requireObject(harness.world, "parent");

        require(nearlyEqual(localValue(runtimeChild, "motionX"), runtimeChild.origin.x), "child motion sees pre-attachment x");
        require(nearlyEqual(localValue(runtimeChild, "collisionX"), runtimeChild.position.x), "collision sees post-attachment x");
        require(nearlyEqual(runtimeChild.position.x, runtimeParent.position.x + runtimeChild.originalOffset.x), "attached x follows parent plus stored original offset");
        require(nearlyEqual(runtimeChild.position.y, runtimeChild.origin.y), "unattached y remains unchanged");
        require(nearlyEqual(runtimeChild.angle, 35.0), "attached angle follows parent angle");
    }

    void testFindByRuntimeIdNameDuplicatesAndPendingLookup()
    {
        RuntimeHarness harness;

        harness.addScript(
            "spawnOnce",
            "function action(o) { if (o.local['done'] == null) { o.local['done'] = 1; spawn(o, 'dup'); } }"
        );

        ObjectDefinition root =
            objectDefinition("root", "spawnOnce");
        root.childResources["dup"] = "dup";

        ObjectDefinition duplicate =
            objectDefinition("dup");
        duplicate.spawnMode = "manual";

        harness.addObject(root);
        harness.addObject(duplicate);

        require(harness.load().success, "runtime should load identity project");

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");
        const std::string rootRuntimeId =
            runtimeRoot.runtimeId;

        require(harness.world.findByRuntimeId(runtimeRoot.runtimeId) == &runtimeRoot, "findByRuntimeId should be exact");

        harness.update();
        RuntimeObject& firstDuplicate =
            requireObject(harness.world, "dup");
        const std::string firstDuplicateRuntimeId =
            firstDuplicate.runtimeId;

        RuntimeObject* rootAfterSpawn =
            harness.world.findByRuntimeId(rootRuntimeId);
        require(rootAfterSpawn != nullptr, "root should still exist after first spawn flush");
        harness.world.spawn(
            *rootAfterSpawn,
            rootAfterSpawn->childResources.at("dup"),
            harness.scripts
        );

        require(harness.world.findByName("dup")->runtimeId == firstDuplicateRuntimeId, "findByName should return first live duplicate before pending flush");

        harness.update();

        require(harness.world.findByName("dup")->runtimeId == firstDuplicateRuntimeId, "findByName should keep first live duplicate after second spawn");

        RuntimeObject* firstDuplicateAfterSecondSpawn =
            harness.world.findByRuntimeId(firstDuplicateRuntimeId);
        require(firstDuplicateAfterSecondSpawn != nullptr, "first duplicate should still exist before cleanup");
        firstDuplicateAfterSecondSpawn->alive = false;
        harness.update();

        require(harness.world.findByRuntimeId(firstDuplicateRuntimeId) == nullptr, "dead object should not be found after cleanup");
    }

    void testScriptErrorsAreLoggedAndRuntimeContinues()
    {
        RuntimeHarness harness;

        harness.addScript(
            "throwEverywhere",
            "function born(o) { throw new Error('born failure'); }"
            "function action(o) { o.local['actionBeforeThrow'] = 1; throw new Error('action failure'); }"
            "function motion(o) { o.local['motionBeforeThrow'] = 1; throw new Error('motion failure'); }"
            "function collision(o, other) { o.local['collisionBeforeThrow'] = 1; throw new Error('collision failure'); }"
            "function draw(o) { o.local['drawBeforeThrow'] = 1; throw new Error('draw failure'); }"
            "function dead(o) { o.local['deadBeforeThrow'] = 1; throw new Error('dead failure'); }"
        );

        ObjectDefinition root =
            objectDefinition("root", "throwEverywhere");
        root.childResources["target"] = "target";
        root.collisionActive = true;
        root.collisionType = "box";
        root.collisionWith.push_back("target");
        root.size = Vector2{ 10.0f, 10.0f };

        ObjectDefinition target =
            objectDefinition("target");
        target.group = "target";
        target.collisionType = "box";
        target.size = Vector2{ 10.0f, 10.0f };

        harness.addObject(root);
        harness.addObject(target);

        RuntimeLoadResult loadResult =
            harness.load();

        require(loadResult.success, "script exception in born should not fail runtime load currently");

        HiddenTestWindow window;

        harness.update();
        harness.draw();

        RuntimeObject& runtimeRoot =
            requireObject(harness.world, "root");

        require(localValue(runtimeRoot, "actionBeforeThrow") == 1.0, "action should apply object changes before logged exception cleanup");
        require(localValue(runtimeRoot, "motionBeforeThrow") == 1.0, "motion should apply object changes before logged exception cleanup");
        require(localValue(runtimeRoot, "collisionBeforeThrow") == 1.0, "collision should apply object changes before logged exception cleanup");
        require(localValue(runtimeRoot, "drawBeforeThrow") == 1.0, "draw should apply object changes before logged exception cleanup");

        runtimeRoot.alive = false;
        harness.update();

        require(harness.world.findByName("root") == nullptr, "dead exception should not prevent cleanup");
    }

    void testAutomaticInstantiationCycleFailsRuntimeLoad()
    {
        RuntimeHarness harness;

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["a"] = "a";

        ObjectDefinition a =
            objectDefinition("a");
        a.childResources["b"] = "b";

        ObjectDefinition b =
            objectDefinition("b");
        b.childResources["a"] = "a";

        harness.addObject(root);
        harness.addObject(a);
        harness.addObject(b);

        RuntimeLoadResult result =
            harness.load();

        require(!result.success, "runtime load should reject automatic instantiation cycle");
        require(result.diagnostics.hasErrors(), "automatic instantiation cycle should produce diagnostics");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "initial load born order and identity", testInitialLoadBornOrderAndIdentity },
        { "spawn during born is stabilized before load returns", testSpawnDuringBornIsStabilizedBeforeLoadReturns },
        { "recursive spawn during born is stabilized before load returns", testRecursiveSpawnDuringBornIsStabilizedBeforeLoadReturns },
        { "spawn during born limit fails load", testSpawnDuringBornLimitFailsLoad },
        { "spawn during action motion and collision phases", testSpawnDuringActionMotionAndCollisionPhases },
        { "kill during action motion and collision is terminal", testKillDuringActionMotionAndCollisionIsTerminal },
        { "alive is read-only from JavaScript", testAliveIsReadOnlyFromJavaScript },
        { "keep_only prevents other objects from resurrecting", testKeepOnlyPreventsOtherObjectsFromResurrecting },
        { "hide suppresses draw but keeps runtime phases and show restores draw", testHideSuppressesDrawButKeepsRuntimePhasesAndShowRestoresDraw },
        { "visible is read-only from JavaScript", testVisibleIsReadOnlyFromJavaScript },
        { "draw callbacks follow stable layer order", testDrawCallbacksFollowStableLayerOrder },
        { "object time state and timers", testObjectTimeStateAndTimers },
        { "attachments apply after motion before collision", testAttachmentsApplyAfterMotionBeforeCollision },
        { "find by runtime id name duplicates and pending lookup", testFindByRuntimeIdNameDuplicatesAndPendingLookup },
        { "script errors are logged and runtime continues", testScriptErrorsAreLoggedAndRuntimeContinues },
        { "automatic instantiation cycle fails runtime load", testAutomaticInstantiationCycleFailsRuntimeLoad }
    };

    for (const auto& test : tests)
    {
        try
        {
            std::cout << "[RUN] " << test.first << std::endl;
            test.second();
            std::cout << "[PASS] " << test.first << std::endl;
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";
            return 1;
        }
    }

    return 0;
}
