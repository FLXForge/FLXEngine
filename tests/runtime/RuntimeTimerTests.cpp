#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProject.h"
#include "../../engine/runtime/RuntimeWorld.h"
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

    void testPlayTimerCreatesAndAbsentQueriesAreFalse()
    {
        RuntimeHarness harness;

        harness.addScript(
            "createProbe",
            "function born(o) {"
            "  play_timer(o, 'missing');"
            "  write_local(o, 'missingActive', timer_active(o, 'missing') ? 1 : 0);"
            "  write_local(o, 'missingPaused', timer_paused(o, 'missing') ? 1 : 0);"
            "  write_local(o, 'missingDone', timer_done(o, 'missing') ? 1 : 0);"
            "  write_local(o, 'missingLeft', timer_left(o, 'missing'));"
            "  play_timer(o, 'life', 1.0);"
            "  write_local(o, 'lifeActive', timer_active(o, 'life') ? 1 : 0);"
            "  write_local(o, 'lifePaused', timer_paused(o, 'life') ? 1 : 0);"
            "  write_local(o, 'lifeDone', timer_done(o, 'life') ? 1 : 0);"
            "  write_local(o, 'lifeLeft', timer_left(o, 'life'));"
            "}"
        );

        harness.addObject(objectDefinition("root", "createProbe"));

        require(harness.load().success, "runtime should load timer create project");

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(root.timers.count("missing") == 0, "play_timer without duration should not create an absent timer");
        require(localValue(root, "missingActive") == 0.0, "absent timer should not be active");
        require(localValue(root, "missingPaused") == 0.0, "absent timer should not be paused");
        require(localValue(root, "missingDone") == 0.0, "absent timer should not be done");
        require(nearlyEqual(localValue(root, "missingLeft"), 0.0), "absent timer should report zero left");
        require(root.timers.count("life") == 1, "play_timer with duration should create a timer");
        require(nearlyEqual(root.timers["life"].duration, 1.0), "created timer should store duration");
        require(nearlyEqual(root.timers["life"].left, 1.0), "created timer should start with full left");
        require(root.timers["life"].status == RuntimeTimerStatus::Running, "created timer should be running");
        require(localValue(root, "lifeActive") == 1.0, "running timer should be active");
        require(localValue(root, "lifePaused") == 0.0, "running timer should not be paused");
        require(localValue(root, "lifeDone") == 0.0, "running timer should not be done");
        require(nearlyEqual(localValue(root, "lifeLeft"), 1.0), "running timer should report left");
    }

    void testPlayTimerOnRunningPreservesElapsed()
    {
        RuntimeHarness harness;

        harness.addScript(
            "runningProbe",
            "function born(o) { play_timer(o, 'shot', 5.0); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'leftBeforeSameDuration', timer_left(o, 'shot'));"
            "    play_timer(o, 'shot', 5.0);"
            "    write_local(o, 'leftAfterSameDuration', timer_left(o, 'shot'));"
            "    play_timer(o, 'shot', 6.0);"
            "    write_local(o, 'leftAfterLongerDuration', timer_left(o, 'shot'));"
            "    play_timer(o, 'shot', 4.0);"
            "    write_local(o, 'leftAfterShorterDuration', timer_left(o, 'shot'));"
            "    write_local(o, 'doneAfterShorterDuration', timer_done(o, 'shot') ? 1 : 0);"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "runningProbe"));

        require(harness.load().success, "runtime should load running timer project");

        harness.update(3.0f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(root, "leftBeforeSameDuration"), 2.0), "second frame action should see elapsed value before decrement");
        require(nearlyEqual(localValue(root, "leftAfterSameDuration"), 2.0), "same duration should preserve elapsed");
        require(nearlyEqual(localValue(root, "leftAfterLongerDuration"), 3.0), "longer duration should add remaining time based on elapsed");
        require(nearlyEqual(localValue(root, "leftAfterShorterDuration"), 1.0), "shorter duration should preserve elapsed");
        require(localValue(root, "doneAfterShorterDuration") == 0.0, "shorter duration above elapsed should stay running");
        require(nearlyEqual(root.timers["shot"].duration, 4.0), "last duration should become total duration");
        require(nearlyEqual(root.timers["shot"].left, 0.75), "timer should decrement at end of the frame after duration changes");
        require(root.timers["shot"].status == RuntimeTimerStatus::Running, "timer should remain running");
    }

    void testPlayTimerDurationShorterThanElapsedMarksDone()
    {
        RuntimeHarness harness;

        harness.addScript(
            "shorterProbe",
            "function born(o) { play_timer(o, 'life', 5.0); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    play_timer(o, 'life', 2.0);"
            "    write_local(o, 'activeAfterShort', timer_active(o, 'life') ? 1 : 0);"
            "    write_local(o, 'doneAfterShort', timer_done(o, 'life') ? 1 : 0);"
            "    write_local(o, 'leftAfterShort', timer_left(o, 'life'));"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "shorterProbe"));

        require(harness.load().success, "runtime should load shorter timer project");

        harness.update(3.0f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(localValue(root, "activeAfterShort") == 0.0, "duration below elapsed should make timer inactive");
        require(localValue(root, "doneAfterShort") == 1.0, "duration below elapsed should mark timer done");
        require(nearlyEqual(localValue(root, "leftAfterShort"), 0.0), "done timer should report zero left");
        require(nearlyEqual(root.timers["life"].duration, 2.0), "done timer should keep new total duration");
        require(root.timers["life"].status == RuntimeTimerStatus::Done, "timer should be done");
    }

    void testPlayTimerDurationEqualElapsedMarksDone()
    {
        RuntimeHarness harness;

        harness.addScript(
            "equalProbe",
            "function born(o) { play_timer(o, 'life', 5.0); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    play_timer(o, 'life', 3.0);"
            "    write_local(o, 'activeAfterEqual', timer_active(o, 'life') ? 1 : 0);"
            "    write_local(o, 'pausedAfterEqual', timer_paused(o, 'life') ? 1 : 0);"
            "    write_local(o, 'doneAfterEqual', timer_done(o, 'life') ? 1 : 0);"
            "    write_local(o, 'leftAfterEqual', timer_left(o, 'life'));"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "equalProbe"));

        require(harness.load().success, "runtime should load equal timer project");

        harness.update(3.0f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(localValue(root, "activeAfterEqual") == 0.0, "duration equal to elapsed should not be active");
        require(localValue(root, "pausedAfterEqual") == 0.0, "duration equal to elapsed should not be paused");
        require(localValue(root, "doneAfterEqual") == 1.0, "duration equal to elapsed should mark done");
        require(nearlyEqual(localValue(root, "leftAfterEqual"), 0.0), "duration equal to elapsed should report zero left");
        require(nearlyEqual(root.timers["life"].duration, 3.0), "equal elapsed timer should keep new duration");
        require(nearlyEqual(root.timers["life"].left, 0.0), "equal elapsed timer should have zero left");
        require(root.timers["life"].status == RuntimeTimerStatus::Done, "equal elapsed timer should be done");
    }

    void testPauseResumeAndPausedDurationChange()
    {
        RuntimeHarness harness;

        harness.addScript(
            "pauseProbe",
            "function born(o) { play_timer(o, 'gate', 2.0); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 1) {"
            "    pause_timer(o, 'gate');"
            "    pause_timer(o, 'gate');"
            "    write_local(o, 'pausedActive', timer_active(o, 'gate') ? 1 : 0);"
            "    write_local(o, 'pausedPaused', timer_paused(o, 'gate') ? 1 : 0);"
            "    write_local(o, 'pausedDone', timer_done(o, 'gate') ? 1 : 0);"
            "    write_local(o, 'pausedLeft', timer_left(o, 'gate'));"
            "  }"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'stillPausedLeft', timer_left(o, 'gate'));"
            "    play_timer(o, 'gate');"
            "    write_local(o, 'resumedPaused', timer_paused(o, 'gate') ? 1 : 0);"
            "  }"
            "  if (read_local(o, 'frame') == 3) {"
            "    pause_timer(o, 'gate');"
            "    play_timer(o, 'gate', 3.0);"
            "    write_local(o, 'leftAfterPausedDurationChange', timer_left(o, 'gate'));"
            "    write_local(o, 'pausedAfterDurationChange', timer_paused(o, 'gate') ? 1 : 0);"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "pauseProbe"));

        require(harness.load().success, "runtime should load pause timer project");

        harness.update(0.25f);
        harness.update(0.25f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(localValue(root, "pausedActive") == 1.0, "paused timer should remain active");
        require(localValue(root, "pausedPaused") == 1.0, "paused timer should report paused");
        require(localValue(root, "pausedDone") == 0.0, "paused timer should not be done");
        require(nearlyEqual(localValue(root, "pausedLeft"), 2.0), "pause should freeze current left in same frame");
        require(nearlyEqual(localValue(root, "stillPausedLeft"), 2.0), "paused timer should not decrement across frames");
        require(localValue(root, "resumedPaused") == 0.0, "play without duration should resume paused timer");
        require(nearlyEqual(localValue(root, "leftAfterPausedDurationChange"), 2.75), "duration change from paused should preserve elapsed");
        require(localValue(root, "pausedAfterDurationChange") == 0.0, "play with duration should resume paused timer");
        require(nearlyEqual(root.timers["gate"].left, 2.5), "running timer should decrement after resume and duration change");
    }

    void testNaturalDonePersistsAndCanReplay()
    {
        RuntimeHarness harness;

        harness.addScript(
            "doneProbe",
            "function born(o) { play_timer(o, 'life', 0.25); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'activeAfterDone', timer_active(o, 'life') ? 1 : 0);"
            "    write_local(o, 'pausedAfterDone', timer_paused(o, 'life') ? 1 : 0);"
            "    write_local(o, 'doneAfterDone', timer_done(o, 'life') ? 1 : 0);"
            "    write_local(o, 'leftAfterDone', timer_left(o, 'life'));"
            "  }"
            "  if (read_local(o, 'frame') == 3) {"
            "    write_local(o, 'doneStillPersistent', timer_done(o, 'life') ? 1 : 0);"
            "    play_timer(o, 'life');"
            "    write_local(o, 'leftAfterReplay', timer_left(o, 'life'));"
            "  }"
            "  if (read_local(o, 'frame') == 4) {"
            "    play_timer(o, 'life', 0.5);"
            "    write_local(o, 'leftAfterDoneNewDuration', timer_left(o, 'life'));"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "doneProbe"));

        require(harness.load().success, "runtime should load done timer project");

        harness.update(0.25f);
        harness.update(0.25f);
        harness.update(0.25f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(localValue(root, "activeAfterDone") == 0.0, "done timer should not be active");
        require(localValue(root, "pausedAfterDone") == 0.0, "done timer should not be paused");
        require(localValue(root, "doneAfterDone") == 1.0, "done timer should report done");
        require(nearlyEqual(localValue(root, "leftAfterDone"), 0.0), "done timer should report zero left");
        require(localValue(root, "doneStillPersistent") == 1.0, "done should persist across frames");
        require(nearlyEqual(localValue(root, "leftAfterReplay"), 0.25), "play without duration should replay done timer from known duration");
        require(nearlyEqual(localValue(root, "leftAfterDoneNewDuration"), 0.5), "play with duration on done should set new duration");
        require(nearlyEqual(root.timers["life"].left, 0.25), "replayed timer should decrement at frame end");
    }

    void testStopRemovesRunningPausedDoneAndAbsentIsNoop()
    {
        RuntimeHarness harness;

        harness.addScript(
            "stopProbe",
            "function born(o) {"
            "  play_timer(o, 'running', 1.0);"
            "  play_timer(o, 'paused', 1.0);"
            "  pause_timer(o, 'paused');"
            "  play_timer(o, 'done', 0.25);"
            "}"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    stop_timer(o, 'running');"
            "    stop_timer(o, 'paused');"
            "    stop_timer(o, 'done');"
            "    stop_timer(o, 'absent');"
            "    write_local(o, 'runningActive', timer_active(o, 'running') ? 1 : 0);"
            "    write_local(o, 'pausedPaused', timer_paused(o, 'paused') ? 1 : 0);"
            "    write_local(o, 'doneDone', timer_done(o, 'done') ? 1 : 0);"
            "    write_local(o, 'doneLeft', timer_left(o, 'done'));"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "stopProbe"));

        require(harness.load().success, "runtime should load stop timer project");

        harness.update(0.25f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(root.timers.empty(), "stop_timer should remove running, paused and done timers");
        require(localValue(root, "runningActive") == 0.0, "stopped running timer should not be active");
        require(localValue(root, "pausedPaused") == 0.0, "stopped paused timer should not be paused");
        require(localValue(root, "doneDone") == 0.0, "stopped done timer should not be done");
        require(nearlyEqual(localValue(root, "doneLeft"), 0.0), "stopped timer should report zero left");
    }

    void testInvalidInputsDoNotModifyTimers()
    {
        RuntimeHarness harness;

        harness.addScript(
            "invalidProbe",
            "function born(o) {"
            "  play_timer(o, 'life', 1.0);"
            "  play_timer(o, '', 0.5);"
            "  play_timer(o, 'zero', 0.0);"
            "  play_timer(o, 'negative', -1.0);"
            "  play_timer(o, 'nan', NaN);"
            "  play_timer(o, 'infinity', Infinity);"
            "  play_timer(o, 'negativeInfinity', -Infinity);"
            "  play_timer(o, 'life', 0.0);"
            "  play_timer(o, 'life', NaN);"
            "  play_timer(o, 'life', Infinity);"
            "  play_timer(o, 'life', -Infinity);"
            "  write_local(o, 'lifeLeft', timer_left(o, 'life'));"
            "}"
        );

        harness.addObject(objectDefinition("root", "invalidProbe"));

        require(harness.load().success, "runtime should load invalid timer project");

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(root.timers.size() == 1, "invalid timer inputs should not create timers");
        require(root.timers.count("life") == 1, "valid timer should remain");
        require(nearlyEqual(localValue(root, "lifeLeft"), 1.0), "invalid duration should not modify existing timer");
        require(nearlyEqual(root.timers["life"].left, 1.0), "existing timer should keep previous left after invalid calls");
    }

    void testInvalidInputsDoNotModifyElapsedTimer()
    {
        RuntimeHarness harness;

        harness.addScript(
            "invalidElapsedProbe",
            "function born(o) { play_timer(o, 'life', 5.0); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'leftBeforeInvalid', timer_left(o, 'life'));"
            "    play_timer(o, '', 10.0);"
            "    play_timer(o, 'life', NaN);"
            "    play_timer(o, 'life', Infinity);"
            "    play_timer(o, 'life', -Infinity);"
            "    play_timer(o, 'life', 0.0);"
            "    play_timer(o, 'life', -1.0);"
            "    write_local(o, 'leftAfterInvalid', timer_left(o, 'life'));"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "invalidElapsedProbe"));

        require(harness.load().success, "runtime should load invalid elapsed timer project");

        harness.update(2.0f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(root, "leftBeforeInvalid"), 3.0), "elapsed timer should expose value before invalid calls");
        require(nearlyEqual(localValue(root, "leftAfterInvalid"), 3.0), "invalid calls should not modify elapsed timer inside callback");
        require(nearlyEqual(root.timers["life"].duration, 5.0), "invalid calls should not modify existing duration");
        require(nearlyEqual(root.timers["life"].left, 2.75), "valid elapsed timer should only receive normal frame decrement");
        require(root.timers["life"].status == RuntimeTimerStatus::Running, "invalid calls should not change existing status");
    }

    void testMultipleTimersAndInstanceIsolation()
    {
        RuntimeHarness harness;

        harness.addScript(
            "multiProbe",
            "function born(o) {"
            "  play_timer(o, 'same', o.name == 'a' ? 1.0 : 2.0);"
            "  play_timer(o, 'other', 3.0);"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["a"] = "a";
        root.childResources["b"] = "b";

        harness.addObject(root);
        harness.addObject(objectDefinition("a", "multiProbe"));
        harness.addObject(objectDefinition("b", "multiProbe"));

        require(harness.load().success, "runtime should load timer isolation project");

        RuntimeObject& a =
            requireObject(harness.world, "a");
        RuntimeObject& b =
            requireObject(harness.world, "b");

        require(a.timers.size() == 2, "one instance should hold multiple named timers");
        require(b.timers.size() == 2, "second instance should hold its own named timers");
        require(nearlyEqual(a.timers["same"].left, 1.0), "first instance same-name timer should use first duration");
        require(nearlyEqual(b.timers["same"].left, 2.0), "second instance same-name timer should use second duration");
    }

    void testTimerOperationsAreIsolated()
    {
        RuntimeHarness harness;

        harness.addScript(
            "isolationProbe",
            "function born(o) {"
            "  play_timer(o, 'same', o.name == 'a' ? 1.0 : 2.0);"
            "  play_timer(o, 'other', 3.0);"
            "}"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2 && o.name == 'a') {"
            "    pause_timer(o, 'same');"
            "    stop_timer(o, 'other');"
            "  }"
            "  if (read_local(o, 'frame') == 2 && o.name == 'b') {"
            "    play_timer(o, 'same', 1.0);"
            "  }"
            "}"
        );

        ObjectDefinition root =
            objectDefinition("root");
        root.childResources["a"] = "a";
        root.childResources["b"] = "b";

        harness.addObject(root);
        harness.addObject(objectDefinition("a", "isolationProbe"));
        harness.addObject(objectDefinition("b", "isolationProbe"));

        require(harness.load().success, "runtime should load timer operation isolation project");

        harness.update(0.5f);
        harness.update(0.25f);

        RuntimeObject& a =
            requireObject(harness.world, "a");
        RuntimeObject& b =
            requireObject(harness.world, "b");

        require(a.timers.count("other") == 0, "stop on one instance should remove only that instance timer");
        require(b.timers.count("other") == 1, "stop on another instance should not remove this instance timer");
        require(a.timers["same"].status == RuntimeTimerStatus::Paused, "pause should affect only selected instance");
        require(nearlyEqual(a.timers["same"].left, 0.5), "paused selected instance should not decrement");
        require(b.timers["same"].status == RuntimeTimerStatus::Running, "duration change on other instance should stay local");
        require(nearlyEqual(b.timers["same"].duration, 1.0), "duration change should affect only other instance");
        require(nearlyEqual(b.timers["same"].left, 0.25), "duration change should preserve elapsed and decrement normally");
        require(nearlyEqual(b.timers["other"].left, 2.25), "unrelated timer should continue normally");
    }

    void testFrameTimingByPhase()
    {
        RuntimeHarness harness;

        harness.addScript(
            "phaseProbe",
            "function born(o) {"
            "  play_timer(o, 'born', 1.0);"
            "  write_local(o, 'bornLeft', timer_left(o, 'born'));"
            "}"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 1) {"
            "    play_timer(o, 'action', 1.0);"
            "    write_local(o, 'actionLeftSameCallback', timer_left(o, 'action'));"
            "    write_local(o, 'bornLeftInAction', timer_left(o, 'born'));"
            "  }"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'bornLeftFrame2', timer_left(o, 'born'));"
            "    write_local(o, 'actionLeftFrame2', timer_left(o, 'action'));"
            "    write_local(o, 'motionLeftFrame2', timer_left(o, 'motion'));"
            "    write_local(o, 'collisionLeftFrame2', timer_left(o, 'collision'));"
            "  }"
            "}"
            "function motion(o) {"
            "  if (read_local(o, 'frame') == 1) {"
            "    play_timer(o, 'motion', 1.0);"
            "    write_local(o, 'motionLeftSameCallback', timer_left(o, 'motion'));"
            "  }"
            "}"
            "function collision(o, other) {"
            "  if (read_local(o, 'frame') == 1) {"
            "    play_timer(o, 'collision', 1.0);"
            "    write_local(o, 'collisionLeftSameCallback', timer_left(o, 'collision'));"
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

    void testLifecycleHideStateKillAndDead()
    {
        RuntimeHarness harness;

        harness.addScript(
            "rootProbe",
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'deadSawLeft', read_global('deadSawLeft') || 0);"
            "    write_local(o, 'deadSawActive', read_global('deadSawActive') || 0);"
            "  }"
            "}"
        );

        harness.addScript(
            "victimProbe",
            "function born(o) { play_timer(o, 'life', 1.0); }"
            "function action(o) { kill(o); }"
            "function dead(o) {"
            "  write_global('deadSawLeft', timer_left(o, 'life'));"
            "  write_global('deadSawActive', timer_active(o, 'life') ? 1 : 0);"
            "}"
        );

        harness.addScript(
            "hiddenStateProbe",
            "function born(o) { play_timer(o, 'life', 1.0); hide(o); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 1) {"
            "    state_to(o, 'done');"
            "    write_local(o, 'leftBeforeStateFrameEnd', timer_left(o, 'life'));"
            "  }"
            "  if (read_local(o, 'frame') == 2) {"
            "    write_local(o, 'leftAfterHiddenState', timer_left(o, 'life'));"
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

    void testHistoricalRestartUsesExplicitStopThenPlay()
    {
        RuntimeHarness harness;

        harness.addScript(
            "restartProbe",
            "function born(o) { play_timer(o, 'powerup', 5.0); }"
            "function action(o) {"
            "  write_local(o, 'frame', (read_local(o, 'frame') || 0) + 1);"
            "  if (read_local(o, 'frame') == 2) {"
            "    play_timer(o, 'powerup', 5.0);"
            "    write_local(o, 'leftAfterPlainPlay', timer_left(o, 'powerup'));"
            "    stop_timer(o, 'powerup');"
            "    play_timer(o, 'powerup', 5.0);"
            "    write_local(o, 'leftAfterExplicitRestart', timer_left(o, 'powerup'));"
            "  }"
            "}"
        );

        harness.addObject(objectDefinition("root", "restartProbe"));

        require(harness.load().success, "runtime should load restart timer project");

        harness.update(2.0f);
        harness.update(0.25f);

        RuntimeObject& root =
            requireObject(harness.world, "root");

        require(nearlyEqual(localValue(root, "leftAfterPlainPlay"), 3.0), "play_timer with duration should preserve elapsed, not restart");
        require(nearlyEqual(localValue(root, "leftAfterExplicitRestart"), 5.0), "stop_timer plus play_timer should express explicit restart");
        require(nearlyEqual(root.timers["powerup"].left, 4.75), "explicitly restarted timer should decrement at frame end");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "play timer creates and absent queries are false", testPlayTimerCreatesAndAbsentQueriesAreFalse },
        { "play timer on running preserves elapsed", testPlayTimerOnRunningPreservesElapsed },
        { "play timer duration shorter than elapsed marks done", testPlayTimerDurationShorterThanElapsedMarksDone },
        { "play timer duration equal elapsed marks done", testPlayTimerDurationEqualElapsedMarksDone },
        { "pause resume and paused duration change", testPauseResumeAndPausedDurationChange },
        { "natural done persists and can replay", testNaturalDonePersistsAndCanReplay },
        { "stop removes running paused done and absent is noop", testStopRemovesRunningPausedDoneAndAbsentIsNoop },
        { "invalid inputs do not modify timers", testInvalidInputsDoNotModifyTimers },
        { "invalid inputs do not modify elapsed timer", testInvalidInputsDoNotModifyElapsedTimer },
        { "multiple timers and instance isolation", testMultipleTimersAndInstanceIsolation },
        { "timer operations are isolated", testTimerOperationsAreIsolated },
        { "frame timing by phase", testFrameTimingByPhase },
        { "lifecycle hide state kill and dead", testLifecycleHideStateKillAndDead },
        { "historical restart uses explicit stop then play", testHistoricalRestartUsesExplicitStopThenPlay }
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
