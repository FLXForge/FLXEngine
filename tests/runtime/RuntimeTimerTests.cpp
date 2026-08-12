#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
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

    void testTimerCreationRestartAndMultipleNames()
    {
        RuntimeHarness harness;

        harness.addScript(
            "timerProbe",
            "function born(o) {"
            "  timer(o, 'shot', 1.0);"
            "  timer(o, 'shield', 2.0);"
            "  timer(o, 'shot', 3.0);"
            "  o.local['shotLeftBorn'] = timer_left(o, 'shot');"
            "  o.local['shieldLeftBorn'] = timer_left(o, 'shield');"
            "  o.local['shotActiveBorn'] = timer_active(o, 'shot') ? 1 : 0;"
            "}"
        );

        harness.addObject(objectDefinition("root", "timerProbe"));

        require(harness.load().success, "runtime should load timer creation project");

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(root.timers.size() == 2, "object should own two named timers");
        require(nearlyEqual(localValue(root, "shotLeftBorn"), 3.0), "timer() should restart same name with new left value");
        require(nearlyEqual(localValue(root, "shieldLeftBorn"), 2.0), "different timer names should coexist");
        require(localValue(root, "shotActiveBorn") == 1.0, "positive timer should be active immediately");

        harness.update(0.5f);

        RuntimeObject& updatedRoot =
            requireObject(harness.world, "root");

        require(nearlyEqual(updatedRoot.timers["shot"].left, 2.5), "restarted timer should decrement from replacement value");
        require(nearlyEqual(updatedRoot.timers["shield"].left, 1.5), "second timer should decrement independently");
    }

    void testTimerQueriesAndClear()
    {
        RuntimeHarness harness;

        harness.addScript(
            "clearProbe",
            "function born(o) {"
            "  o.local['missingActive'] = timer_active(o, 'missing') ? 1 : 0;"
            "  o.local['missingLeft'] = timer_left(o, 'missing');"
            "  timer(o, 'done', 0.0);"
            "  o.local['zeroActive'] = timer_active(o, 'done') ? 1 : 0;"
            "  o.local['zeroLeft'] = timer_left(o, 'done');"
            "  timer(o, 'gone', 1.0);"
            "  timer_clear(o, 'gone');"
            "  timer_clear(o, 'missing');"
            "  o.local['goneActive'] = timer_active(o, 'gone') ? 1 : 0;"
            "  o.local['goneLeft'] = timer_left(o, 'gone');"
            "}"
        );

        harness.addObject(objectDefinition("root", "clearProbe"));

        require(harness.load().success, "runtime should load clear timer project");

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(localValue(root, "missingActive") == 0.0, "missing timer should not be active");
        require(nearlyEqual(localValue(root, "missingLeft"), 0.0), "missing timer should report zero left");
        require(root.timers.count("done") == 1, "zero-duration timer should exist");
        require(localValue(root, "zeroActive") == 0.0, "zero-duration timer should not be active");
        require(nearlyEqual(localValue(root, "zeroLeft"), 0.0), "zero-duration timer should report zero left");
        require(root.timers.count("gone") == 0, "timer_clear should erase existing timer");
        require(localValue(root, "goneActive") == 0.0, "cleared timer should not be active");
        require(nearlyEqual(localValue(root, "goneLeft"), 0.0), "cleared timer should report zero left");
    }

    void testTimerBoundaryDurations()
    {
        RuntimeHarness harness;

        harness.addScript(
            "boundaryProbe",
            "function born(o) {"
            "  timer(o, 'negative', -1.0);"
            "  timer(o, '', 0.5);"
            "  timer(o, 'nan', NaN);"
            "  timer(o, 'infinity', Infinity);"
            "  o.local['negativeActive'] = timer_active(o, 'negative') ? 1 : 0;"
            "  o.local['negativeLeft'] = timer_left(o, 'negative');"
            "  o.local['emptyActive'] = timer_active(o, '') ? 1 : 0;"
            "  o.local['emptyLeft'] = timer_left(o, '');"
            "  o.local['nanActive'] = timer_active(o, 'nan') ? 1 : 0;"
            "  o.local['nanLeft'] = timer_left(o, 'nan');"
            "  o.local['infinityActive'] = timer_active(o, 'infinity') ? 1 : 0;"
            "}"
        );

        harness.addObject(objectDefinition("root", "boundaryProbe"));

        require(harness.load().success, "runtime should load timer boundary project");

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(root.timers.count("negative") == 1, "negative duration should create a timer");
        require(localValue(root, "negativeActive") == 0.0, "negative duration should clamp to inactive zero");
        require(nearlyEqual(localValue(root, "negativeLeft"), 0.0), "negative duration should clamp left to zero");
        require(root.timers.count("") == 1, "empty timer name should be accepted as a key");
        require(localValue(root, "emptyActive") == 1.0, "empty-name timer should be active when positive");
        require(nearlyEqual(localValue(root, "emptyLeft"), 0.5), "empty-name timer should keep its duration");
        require(root.timers.count("nan") == 1, "NaN duration should create a timer");
        require(localValue(root, "nanActive") == 0.0, "NaN duration should behave as inactive zero today");
        require(nearlyEqual(localValue(root, "nanLeft"), 0.0), "NaN duration should store zero today");
        require(root.timers.count("infinity") == 1, "Infinity duration should create a timer");
        require(localValue(root, "infinityActive") == 1.0, "Infinity duration should be active today");
        require(std::isinf(root.timers["infinity"].left), "Infinity duration should store infinity today");
    }

    void testTimerFrameTimingByPhase()
    {
        RuntimeHarness harness;

        harness.addScript(
            "phaseProbe",
            "function born(o) {"
            "  timer(o, 'born', 1.0);"
            "  o.local['bornLeft'] = timer_left(o, 'born');"
            "}"
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 1) {"
            "    timer(o, 'action', 1.0);"
            "    o.local['actionLeftSameCallback'] = timer_left(o, 'action');"
            "    o.local['bornLeftInAction'] = timer_left(o, 'born');"
            "  }"
            "  if (o.local['frame'] == 2) {"
            "    o.local['bornLeftFrame2'] = timer_left(o, 'born');"
            "    o.local['actionLeftFrame2'] = timer_left(o, 'action');"
            "    o.local['motionLeftFrame2'] = timer_left(o, 'motion');"
            "    o.local['collisionLeftFrame2'] = timer_left(o, 'collision');"
            "  }"
            "}"
            "function motion(o) {"
            "  if (o.local['frame'] == 1) {"
            "    timer(o, 'motion', 1.0);"
            "    o.local['motionLeftSameCallback'] = timer_left(o, 'motion');"
            "  }"
            "}"
            "function collision(o, other) {"
            "  if (o.local['frame'] == 1) {"
            "    timer(o, 'collision', 1.0);"
            "    o.local['collisionLeftSameCallback'] = timer_left(o, 'collision');"
            "  }"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "phaseProbe");
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

        require(harness.load().success, "runtime should load timer phase project");

        RuntimeObject& loadedRoot =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(loadedRoot, "bornLeft"), 1.0), "born should observe newly created timer before any decrement");
        require(nearlyEqual(loadedRoot.timers["born"].left, 1.0), "born-created timer should remain full until first update");

        harness.update(0.25f);

        RuntimeObject& afterFirstFrame =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(afterFirstFrame, "bornLeftInAction"), 1.0), "action should see born timer before frame decrement");
        require(nearlyEqual(localValue(afterFirstFrame, "actionLeftSameCallback"), 1.0), "action-created timer should be full in same callback");
        require(nearlyEqual(localValue(afterFirstFrame, "motionLeftSameCallback"), 1.0), "motion-created timer should be full in same callback");
        require(nearlyEqual(localValue(afterFirstFrame, "collisionLeftSameCallback"), 1.0), "collision-created timer should be full in same callback");
        require(nearlyEqual(afterFirstFrame.timers["born"].left, 0.75), "born timer should decrement after collision phase");
        require(nearlyEqual(afterFirstFrame.timers["action"].left, 0.75), "action-created timer should decrement in same frame after collision");
        require(nearlyEqual(afterFirstFrame.timers["motion"].left, 0.75), "motion-created timer should decrement in same frame after collision");
        require(nearlyEqual(afterFirstFrame.timers["collision"].left, 0.75), "collision-created timer should decrement in same frame after collision");

        harness.update(0.25f);

        RuntimeObject& afterSecondFrame =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(afterSecondFrame, "bornLeftFrame2"), 0.75), "next action should see previous frame timer value before second decrement");
        require(nearlyEqual(localValue(afterSecondFrame, "actionLeftFrame2"), 0.75), "next action should see action timer before second decrement");
        require(nearlyEqual(localValue(afterSecondFrame, "motionLeftFrame2"), 0.75), "next action should see motion timer before second decrement");
        require(nearlyEqual(localValue(afterSecondFrame, "collisionLeftFrame2"), 0.75), "next action should see collision timer before second decrement");
        require(nearlyEqual(afterSecondFrame.timers["born"].left, 0.5), "timers should decrement once per update");
    }

    void testFinishedTimersPersistAtZeroUntilClearedOrRestarted()
    {
        RuntimeHarness harness;

        harness.addScript(
            "finishProbe",
            "function born(o) { timer(o, 'life', 0.25); }"
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 2) {"
            "    o.local['activeAfterFinish'] = timer_active(o, 'life') ? 1 : 0;"
            "    o.local['leftAfterFinish'] = timer_left(o, 'life');"
            "    timer(o, 'life', 0.5);"
            "    o.local['leftAfterRestart'] = timer_left(o, 'life');"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "finishProbe"));

        require(harness.load().success, "runtime should load finished timer project");

        harness.update(0.25f);

        RuntimeObject& afterFinish =
            requireObject(harness.world, "root");

        require(afterFinish.timers.count("life") == 1, "finished timer should remain stored");
        require(nearlyEqual(afterFinish.timers["life"].left, 0.0), "finished timer should clamp to zero");

        harness.update(0.25f);

        RuntimeObject& afterRestart =
            requireObject(harness.world, "root");

        require(localValue(afterRestart, "activeAfterFinish") == 0.0, "finished timer should not be active");
        require(nearlyEqual(localValue(afterRestart, "leftAfterFinish"), 0.0), "finished timer should report zero before restart");
        require(nearlyEqual(localValue(afterRestart, "leftAfterRestart"), 0.5), "timer() should restart finished timer immediately");
        require(nearlyEqual(afterRestart.timers["life"].left, 0.25), "restarted timer should decrement at end of same frame");
    }

    void testTimerInteractionWithKillDeadHideAndState()
    {
        RuntimeHarness harness;

        harness.addScript(
            "rootProbe",
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 2) {"
            "    o.local['deadSawLeft'] = global['deadSawLeft'] || 0;"
            "    o.local['deadSawActive'] = global['deadSawActive'] || 0;"
            "  }"
            "}"
        );

        harness.addScript(
            "victimProbe",
            "function born(o) { timer(o, 'life', 1.0); }"
            "function action(o) { kill(o); }"
            "function dead(o) {"
            "  global['deadSawLeft'] = timer_left(o, 'life');"
            "  global['deadSawActive'] = timer_active(o, 'life') ? 1 : 0;"
            "}"
        );

        harness.addScript(
            "hiddenStateProbe",
            "function born(o) { timer(o, 'life', 1.0); hide(o); }"
            "function action(o) {"
            "  o.local['frame'] = (o.local['frame'] || 0) + 1;"
            "  if (o.local['frame'] == 1) {"
            "    state_to(o, 'done');"
            "    o.local['leftBeforeStateFrameEnd'] = timer_left(o, 'life');"
            "  }"
            "  if (o.local['frame'] == 2) {"
            "    o.local['leftAfterHiddenState'] = timer_left(o, 'life');"
            "  }"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root", "rootProbe");
        root.childResources["victim"] = "victim";
        root.childResources["hiddenState"] = "hiddenState";

        ObjectDefinition victim =
            objectDefinition("victim", "victimProbe");

        ObjectDefinition hiddenState =
            objectDefinition("hiddenState", "hiddenStateProbe");
        hiddenState.initialState = "start";
        hiddenState.stateTransitions["start"] = { "done" };
        hiddenState.stateTransitions["done"] = {};

        harness.addObject(root);
        harness.addObject(victim);
        harness.addObject(hiddenState);

        require(harness.load().success, "runtime should load timer interaction project");

        harness.update(0.25f);

        require(harness.world.findByName("victim") == nullptr, "killed object should be cleaned after dead");

        RuntimeObject& hiddenAfterFirstFrame =
            requireObject(harness.world, "hiddenState");

        require(!hiddenAfterFirstFrame.visible, "hide should mark timer owner invisible");
        require(nearlyEqual(localValue(hiddenAfterFirstFrame, "leftBeforeStateFrameEnd"), 1.0), "state_to should not change timer immediately");
        require(nearlyEqual(hiddenAfterFirstFrame.timers["life"].left, 0.75), "hidden object timer should decrement after frame");

        harness.update(0.25f);

        RuntimeObject& rootAfterSecondFrame =
            requireObject(harness.world, "root");
        RuntimeObject& hiddenAfterSecondFrame =
            requireObject(harness.world, "hiddenState");

        require(nearlyEqual(localValue(rootAfterSecondFrame, "deadSawLeft"), 1.0), "dead callback should see killed object timer before decrement");
        require(localValue(rootAfterSecondFrame, "deadSawActive") == 1.0, "dead callback should be able to query active timer before cleanup");
        require(nearlyEqual(localValue(hiddenAfterSecondFrame, "leftAfterHiddenState"), 0.75), "hidden object should observe previous timer value before next decrement");
        require(nearlyEqual(hiddenAfterSecondFrame.timers["life"].left, 0.5), "hidden object timer should continue after state transition");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "timer creation restart and multiple names", testTimerCreationRestartAndMultipleNames },
        { "timer queries and clear", testTimerQueriesAndClear },
        { "timer boundary durations", testTimerBoundaryDurations },
        { "timer frame timing by phase", testTimerFrameTimingByPhase },
        { "finished timers persist at zero until cleared or restarted", testFinishedTimersPersistAtZeroUntilClearedOrRestarted },
        { "timer interaction with kill dead hide and state", testTimerInteractionWithKillDeadHideAndState }
    };

    for (const auto& test : tests)
    {
        try
        {
            test.second();
            std::cout << "[PASS] " << test.first << "\n";
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";
            return 1;
        }
    }

    return 0;
}
