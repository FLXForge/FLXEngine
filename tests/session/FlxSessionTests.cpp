#include "../support/TestSupport.h"
#include "SessionTestProject.h"
#include <core/FlxSession.h>
#include <input/InputSystem.h>
#include <runtime/AttachmentRuntimeState.h>
#include <runtime/RuntimeObject.h>
#include <compiler/CompiledProjectBinary.h>
#include <cmath>
#include <limits>
#include <iostream>
#include <vector>

using namespace flx::test;
namespace
{
    class Input : public PhysicalInputProvider
    {
    public:
        bool held = false, fail = false;
        bool keyDown(int key) const override
        {
            if (fail) throw std::runtime_error("host input failure");
            return held && key == KEY_SPACE;
        }
        bool keyPressed(int) const override { return false; }
        bool gamepadButtonDown(int, int) const override { return false; }
        bool gamepadButtonPressed(int, int) const override { return false; }
    };
    FlxSessionOptions logical(Input* input = nullptr)
    {
        FlxSessionOptions options;
        options.enableVideo = options.enableAudio = false;
        options.inputProvider = input;
        return options;
    }
    double number(const FlxSession& session, const std::string& key)
    {
        auto value = session.testGlobalValue(key);
        require(value.has_value(), "global must exist: " + key);
        if (auto v = std::get_if<double>(&*value)) return *v;
        if (auto v = std::get_if<bool>(&*value)) return *v ? 1 : 0;
        throw std::runtime_error("numeric global expected");
    }
    void lifecycleAndOwnership()
    {
        FlxSession session;
        require(session.state() == FlxSessionState::Empty && !session.finished(), "initial state");
        require(!session.update(0) && !session.render(), "empty operations rejected");
        session.unload(); session.requestStop();
        {
            auto project = sessionTestProject("let visits=0; function action(o){ visits++; write_global('visits', visits); position(o,o.x+delta(),o.y); }");
            require(session.load(project, logical()), "logical load");
            project = {}; // source data must no longer be needed
        }
        require(session.state() == FlxSessionState::Ready, "loaded state");
        require(!session.load(sessionTestProject(), logical()), "loaded project cannot be replaced");
        require(session.update(0.25f), "host delta update");
        require(session.state() == FlxSessionState::Running, "running state");
        require(session.testObject("root")->position.x == 32.25f, "embedded delta is not standalone-clamped");
        require(number(session, "visits") == 1, "owned script executed");
        require(!session.render() && session.output().id == 0, "logical mode has no surface");
        session.requestStop(); session.requestStop();
        require(session.finished() && session.result().exitReason == FlxSessionExitReason::HostRequestedStop, "host stop");
        require(session.result().framesExecuted == 1 && !session.update(0), "finished session cannot execute again");
        session.unload(); session.unload();
        require(session.state() == FlxSessionState::Empty, "repeatable unload");
        require(session.load(sessionTestProject("let visits=0; function action(o){write_global('visits', ++visits);}"), logical()), "reload");
        require(!session.testGlobalValue("visits"), "new execution has no previous globals");
        require(session.update(0) && number(session, "visits") == 1, "fresh JS context");
    }
    void errorsAndRecovery()
    {
        FlxSession session;
        auto invalid = sessionTestProject(); invalid.rootId = "absent";
        require(!session.load(invalid, logical()), "bad root rejected");
        require(session.state() == FlxSessionState::Failed && session.result().diagnostics.hasErrors(), "failed load diagnostics");
        require(!session.load(sessionTestProject(), logical()), "failed session needs explicit unload");
        session.unload();
        require(session.load(sessionTestProject(), logical()), "recovery after invalid project");
        require(!session.update(std::numeric_limits<float>::quiet_NaN()), "invalid delta rejected");
        require(session.state() == FlxSessionState::Failed && session.output().id == 0, "failed execution releases resources");
        require(session.result().exitReason == FlxSessionExitReason::RuntimeFailed, "runtime failure differs from load failure");
        session.unload(); session.unload();
        Input input; input.fail = true;
        require(session.load(sessionTestProject(), logical(&input)), "load with host provider");
        require(!session.update(0), "host provider exception becomes runtime failure");
        require(session.result().diagnostics.hasErrors(), "runtime exception is diagnosed");
        session.unload(); input.fail = false;
        require(session.load(sessionTestProject(), logical(&input)) && session.update(0), "host usable after failure");
        session.unload();
        auto options = logical(); options.enableVideo = true;
        require(!session.load(sessionTestProject(), options), "video needs host graphics context");
        require(!IsWindowReady(), "session must never initialize window");
        session.unload(); options = logical(); options.enableAudio = true;
        require(!session.load(sessionTestProject(), options), "audio needs host device");
        require(!IsAudioDeviceReady(), "session must never initialize audio device");
        session.unload();
        auto videoInvalid = sessionTestProject(); videoInvalid.context.machine.video.screenHeight = 0;
        require(!session.load(videoInvalid, logical()), "invalid logical size rejected");
    }
    void exitsAndScriptErrors()
    {
        FlxSession session;
        require(session.load(sessionTestProject("function born(o){exit();write_global('after', 1);}"), logical()), "exit from born");
        require(session.finished() && number(session, "after") == 1, "exit must not destroy active callback");
        require(session.result().framesExecuted == 0, "born exit executes no frames");
        session.unload();
        require(session.load(sessionTestProject("function action(o){exit();write_global('after', 2);}"), logical()), "action exit load");
        require(session.update(0) && session.finished() && number(session, "after") == 2, "exit from action is deferred safely");
        require(session.result().exitReason == FlxSessionExitReason::ScriptRequestedExit && session.result().framesExecuted == 0, "script exit reason and frame boundary");
        session.unload();
        require(session.load(sessionTestProject("function born(o){throw Error('expected born failure');} function action(o){throw Error('expected action failure');}"), logical()), "JS exceptions retain audited non-fatal behavior");
        require(session.update(0) && !session.finished(), "JS exception must not silently become fatal");
    }
    void inputAndAttachmentIsolation()
    {
        Input input;
        FlxSession session;
        auto project = sessionTestProject("function action(o){write_global('pressed',input_pressed(player(1),button(0)));write_global('released',input_released(player(1),button(0)));}");
        require(session.load(project, logical(&input)), "input project load");
        input.held = true;
        session.update(0); require(number(session, "pressed") == 1, "first logical press");
        session.update(0); require(number(session, "pressed") == 0, "held button is not repeatedly pressed");
        session.unload(); input.held = false;
        require(session.load(project, logical(&input)), "second input execution");
        session.update(0); require(number(session, "released") == 0, "no release inherited from previous session");
        input.held = true; session.update(0); require(number(session, "pressed") == 1, "new press after reload");
        session.unload();
        require(session.load(project, logical(&input)), "held physical state after reload");
        session.update(0); require(number(session, "pressed") == 1, "physical state is observable through fresh logical state");
        auto* root = session.testObject("root");
        AttachmentRuntimeState::setOffset(*root, { 99, 88 });
        const auto firstId = root->runtimeId;
        session.unload();
        require(session.load(project, logical(&input)), "reload objects");
        root = session.testObject("root");
        require(root->runtimeId == firstId && !root->attachmentOffset, "same generated identity has independent attachment state");
    }
    void binaryExecutionWithoutSources()
    {
        auto project = sessionTestProject("function action(o){write_global('binary',3);exit();}");
        const auto path = testRoot() / "session-binary.flxc";
        Diagnostics diagnostics;
        require(CompiledProjectWriter::write(path.string(), project, diagnostics), "write compiled session project");
        auto loaded = CompiledProjectReader::read(path.string());
        require(loaded.success, "read compiled session project");
        project = {};
        FlxSession session;
        require(session.load(loaded.project, logical()), "load binary project");
        loaded = {};
        require(session.update(0) && session.finished() && number(session, "binary") == 3, "binary execution does not need source files");
    }
}
int main()
{
    try
    {
        lifecycleAndOwnership(); errorsAndRecovery(); exitsAndScriptErrors();
        inputAndAttachmentIsolation(); binaryExecutionWithoutSources();
        std::cout << "[PASS] session lifecycle, ownership, recovery, JS, input and binary execution\n";
    }
    catch (const std::exception& error) { std::cerr << "[FAIL] " << error.what() << '\n'; return 1; }
}
