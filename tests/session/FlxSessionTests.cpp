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
#include <fstream>

using namespace flx::test;
namespace
{
    void savePaths()
    {
        struct Sandbox
        {
            std::filesystem::path cwd = std::filesystem::current_path();
            std::filesystem::path root = testRoot() / "session-save-paths";
            Sandbox() { std::filesystem::remove_all(root); std::filesystem::create_directories(root); std::filesystem::current_path(root); }
            ~Sandbox() { std::filesystem::current_path(cwd); std::error_code ec; std::filesystem::remove_all(root, ec); }
        } sandbox;
        const auto writer = sessionTestProject("function born(o){save('slot','k',true);write_global('value',load('slot','k',false));}");
        const auto reader = sessionTestProject("function born(o){write_global('value',load('slot','k',false));}");
        auto value = [](const FlxSession& s) { return std::get<bool>(*s.testGlobalValue("value")); };
        auto options = FlxSessionOptions{};
        options.enableVideo = options.enableAudio = false;
        FlxSession first, second;
        require(first.load(writer, options) && value(first), "default save/load");
        require(std::filesystem::exists(sandbox.root / "saves/slot.flxsave"), "portable default");
        first.unload();
        options.savePath = sandbox.root / "absolute/nested";
        require(first.load(writer, options) && value(first), "absolute directory creation");
        require(std::filesystem::exists(*options.savePath / "slot.flxsave"), "absolute output");
        first.unload();
        options.savePath = "relative";
        // Resolve at load, before any script action, not at each save/load.
        require(first.load(sessionTestProject("function action(o){save('slot','k',true);write_global('value',load('slot','k',false));}"), options), "relative load");
        std::filesystem::create_directory("other");
        std::filesystem::current_path(sandbox.root / "other");
        require(first.update(0) && value(first), "cwd changes do not redirect persistence");
        require(std::filesystem::exists(sandbox.root / "relative/slot.flxsave"), "relative captured once");
        require(!std::filesystem::exists(sandbox.root / "other/relative"), "no late resolution");
        first.unload();
        std::filesystem::current_path(sandbox.root);
        options.savePath = sandbox.root / "isolated";
        require(second.load(reader, options) && !value(second), "different sessions isolated");
        second.unload();
        options.savePath = sandbox.root / "relative";
        require(second.load(reader, options) && value(second), "shared directory deliberately shares saves");
        second.unload();
        // Fixed legacy v1 bytes: FLXSAVE1, checksum, XOR payload, one boolean key k.
        writeBinary(sandbox.root / "legacy/slot.flxsave", {70,76,88,83,65,86,69,49,87,238,226,67,91,90,90,90,91,90,90,90,91,90,90,90,49,91,90,90,90,91,90,90,90});
        options.savePath = sandbox.root / "legacy";
        require(first.load(reader, options) && value(first), "legacy format readable");
        first.unload();
        writeFile(sandbox.root / "blocked", "regular file");
        options.savePath = sandbox.root / "blocked/child";
        require(first.load(sessionTestProject("function born(o){save('nofallback','k',true);write_global('value',load('nofallback','k',false));}"), options) && !value(first), "write failure preserves JS default semantics");
        require(!std::filesystem::exists(sandbox.root / "blocked/child"), "failed write has no alternate directory");
        require(!std::filesystem::exists(sandbox.root / "saves/nofallback.flxsave"), "no default fallback file");
        first.unload();
        options.savePath = sandbox.root / "readonly";
        require(first.load(writer, options), "create save before denying writes");
        first.unload();
        const auto lockedFile = *options.savePath / "slot.flxsave";
        struct RestorePermissions
        {
            std::filesystem::path path;
            ~RestorePermissions() { std::error_code ec; std::filesystem::permissions(path, std::filesystem::perms::owner_write, std::filesystem::perm_options::add, ec); }
        } restore{lockedFile};
        std::filesystem::permissions(lockedFile, std::filesystem::perms::owner_write | std::filesystem::perms::group_write | std::filesystem::perms::others_write, std::filesystem::perm_options::remove);
        require(first.load(sessionTestProject("function born(o){save('slot','k',false);write_global('value',load('slot','k',false));}"), options) && value(first), "read-only file rejects write and preserves saved value");
        first.unload();
        options.savePath = std::filesystem::path{};
        require(!first.load(writer, options), "explicit empty host path rejected");
        require(first.result().diagnostics.all().front().code == DiagnosticCode::InvalidSessionConfiguration,
            "initialization exception is attributed to configuration");
        first.unload();
        options.savePath = std::filesystem::path(std::string("bad\0path", 8));
        require(!first.load(writer, options), "embedded NUL host path rejected");
    }
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
        require(session.result().success && !session.result().diagnostics.hasErrors() &&
            session.result().diagnostics.all().back().code == DiagnosticCode::SessionLoadInvalidState,
            "rejected load keeps the running project's result successful");
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
        require(session.result().diagnostics.size() == 1 &&
            session.result().diagnostics.all().front().code == DiagnosticCode::CompiledProjectMissingRoot,
            "original world diagnostic is preserved without a generic duplicate");
        require(!session.load(sessionTestProject(), logical()), "failed session needs explicit unload");
        require(session.result().diagnostics.all().back().code == DiagnosticCode::SessionLoadInvalidState,
            "rejected load has its own diagnostic");
        session.unload();
        require(session.result().diagnostics.size() == 2, "diagnostics survive unload");
        require(session.load(sessionTestProject(), logical()), "recovery after invalid project");
        require(session.result().diagnostics.empty(), "disabled audio does not warn");
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
        require(session.result().diagnostics.all().front().code == DiagnosticCode::WindowInitializationFailed,
            "missing graphics context is fatal");
        require(!IsWindowReady(), "session must never initialize window");
        session.unload(); options = logical(); options.enableAudio = true;
        require(session.load(sessionTestProject(), options), "audio device is optional");
        require(!IsAudioDeviceReady(), "session must never initialize audio device");
        require(session.result().success && !session.result().diagnostics.hasErrors(), "missing audio does not fail load");
        require(session.result().diagnostics.all().front().code == DiagnosticCode::SessionAudioUnavailable &&
            session.result().diagnostics.all().front().severity == DiagnosticSeverity::Warning, "structured audio warning");
        require(session.update(0), "silent session updates");
        session.requestStop();
        session.unload();
        require(session.result().success && session.result().diagnostics.size() == 1, "silent session warning survives unload");
        auto videoInvalid = sessionTestProject(); videoInvalid.context.machine.video.screenHeight = 0;
        require(!session.load(videoInvalid, logical()), "invalid logical size rejected");
        require(session.result().diagnostics.size() == 1 &&
            session.result().diagnostics.all().front().code == DiagnosticCode::InvalidVideoOutputConfiguration,
            "new load replaces diagnostics and identifies invalid video configuration");
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
        savePaths(); lifecycleAndOwnership(); errorsAndRecovery(); exitsAndScriptErrors();
        inputAndAttachmentIsolation(); binaryExecutionWithoutSources();
        std::cout << "[PASS] session lifecycle, ownership, recovery, JS, input and binary execution\n";
    }
    catch (const std::exception& error) { std::cerr << "[FAIL] " << error.what() << '\n'; return 1; }
}
