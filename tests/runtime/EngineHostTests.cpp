#include "../support/TestSupport.h"
#include "../../engine/core/Engine.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/cli/commands/RunCommand.h"
#include "../../engine/cli/commands/RunCompiledCommand.h"

#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    bool hasDiagnosticCode(
        const Diagnostics& diagnostics,
        DiagnosticCode code
    )
    {
        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            if (diagnostic.code == code)
            {
                return true;
            }
        }

        return false;
    }

    CompiledProject minimalCompiledProject()
    {
        CompiledProject project;
        project.rootId = "root";
        project.context.name = "EngineHost";
        project.context.title = "EngineHost";
        project.context.machine.video.screenWidth = 64;
        project.context.machine.video.screenHeight = 64;
        project.context.machine.video.outputScale = 1;

        ObjectDefinition root;
        root.id = "root";

        require(project.resources.addObject("root", root), "root should register");

        return project;
    }

    CompiledProject invalidRuntimeProject()
    {
        CompiledProject project =
            minimalCompiledProject();

        project.rootId =
            "missing";

        return project;
    }

    void testRuntimeWorldLoadInvalidReturnsFailure()
    {
        RuntimeWorld world;
        ScriptEngine scriptEngine;

        RuntimeLoadResult result =
            world.load(
                invalidRuntimeProject(),
                scriptEngine
            );

        require(!result.success, "runtime world should reject invalid compiled project");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectMissingRoot), "runtime world load should preserve compiled project diagnostic");
    }

    void testEngineDoesNotEnterLoopWhenRuntimeFails()
    {
        Engine engine;
        RunOptions options;
        options.maxFrames = 3;

        EngineResult result =
            engine.run(
                invalidRuntimeProject(),
                options
            );

        require(!result.success, "engine should fail when runtime world cannot load");
        require(result.exitReason == EngineExitReason::RuntimeLoadFailed, "engine should report runtime load failure");
        require(result.framesExecuted == 0, "engine should not enter loop after runtime load failure");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::RuntimeWorldLoadFailed), "engine should report runtime world load failure diagnostic");
    }

    void testFrameLimitExitReason()
    {
        Engine engine;
        RunOptions options;
        options.maxFrames = 2;

        EngineResult result =
            engine.run(
                minimalCompiledProject(),
                options
            );

        require(result.success, "frame limited engine run should succeed");
        require(result.exitReason == EngineExitReason::FrameLimitReached, "engine should report frame limit");
        require(result.framesExecuted == 2, "engine should count executed frames");
    }

    void testScriptRequestedExitReason()
    {
        const std::filesystem::path root =
            testRoot() / "engine_script_exit";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "scripts");

        writeFile(
            root / "game.flx",
            "name=EngineScriptExit\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"behavior\": { \"scripts\": [\"scripts/root\"] } }\n"
        );

        writeFile(
            root / "game" / "scripts" / "root.js",
            "function action(root) { exit(); }\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "script exit project should compile");

        Engine engine;
        RunOptions options;
        options.maxFrames = 10;

        EngineResult result =
            engine.run(
                compiled.project,
                options
            );

        require(result.success, "script requested exit should be a normal engine exit");
        require(result.exitReason == EngineExitReason::ScriptRequestedExit, "engine should report script requested exit");
        require(result.framesExecuted == 0, "script exit before draw should not count a rendered frame");
    }

    void testSecondRunFails()
    {
        Engine engine;
        RunOptions options;
        options.maxFrames = 1;

        EngineResult first =
            engine.run(
                minimalCompiledProject(),
                options
            );

        require(first.success, "first run should succeed");

        EngineResult second =
            engine.run(
                minimalCompiledProject(),
                options
            );

        require(!second.success, "second run on same engine should fail");
        require(second.exitReason == EngineExitReason::InitializationFailed, "second run should fail during initialization contract");
        require(hasDiagnosticCode(second.diagnostics, DiagnosticCode::EngineAlreadyRun), "second run should use stable diagnostic");
    }

    void testInvalidDimensionsFailBeforeWindow()
    {
        CompiledProject project =
            minimalCompiledProject();

        project.context.machine.video.screenWidth = 0;

        Engine engine;
        RunOptions options;
        options.maxFrames = 1;

        EngineResult result =
            engine.run(
                project,
                options
            );

        require(!result.success, "invalid dimensions should fail");
        require(result.exitReason == EngineExitReason::InitializationFailed, "invalid dimensions should fail initialization");
        require(result.framesExecuted == 0, "invalid dimensions should not enter loop");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::InvalidVideoOutputConfiguration), "invalid dimensions should use video output diagnostic");
    }

    void testScaleOverflowFailsBeforeWindow()
    {
        CompiledProject project =
            minimalCompiledProject();

        project.context.machine.video.screenWidth =
            std::numeric_limits<int>::max();

        RunOptions options;
        options.maxFrames = 1;
        options.scaleOverride = 2;

        Engine engine;
        EngineResult result =
            engine.run(
                project,
                options
            );

        require(!result.success, "overflow dimensions should fail");
        require(result.exitReason == EngineExitReason::InitializationFailed, "overflow should fail initialization");
        require(result.framesExecuted == 0, "overflow should not enter loop");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::InvalidVideoOutputConfiguration), "overflow should use video output diagnostic");
    }

    void testCliPropagatesEngineFailure()
    {
        const std::filesystem::path root =
            testRoot() / "engine_cli_failure";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=EngineCliFailure\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{}\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "cli failure project should compile");

        compiled.project.context.machine.video.screenWidth = 0;

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "invalid runtime config should still be serializable"
        );

        CliArguments runCompiledArguments;
        runCompiledArguments.command = CliCommand::RunCompiled;
        runCompiledArguments.target = output;
        runCompiledArguments.runOptions.maxFrames = 1;

        CompiledProjectBinaryResult readBack =
            CompiledProjectReader::read(output.generic_string());
        require(readBack.success, "invalid runtime config should still be readable");

        const int runCompiledExitCode =
            RunCompiledCommand().execute(runCompiledArguments);

        require(runCompiledExitCode == static_cast<int>(CliExitCode::RuntimeInitializationError), "run-compiled should propagate engine failure");

        CliArguments runArguments;
        runArguments.command = CliCommand::Run;
        runArguments.target = root / "game.flx";
        runArguments.runOptions.maxFrames = 1;
        runArguments.runOptions.scaleOverride = 0;

        const int runExitCode =
            RunCommand().execute(runArguments);

        require(runExitCode == static_cast<int>(CliExitCode::RuntimeInitializationError), "run should propagate engine failure");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "RuntimeWorld load invalid returns failure", testRuntimeWorldLoadInvalidReturnsFailure },
        { "Engine does not enter loop when Runtime fails", testEngineDoesNotEnterLoopWhenRuntimeFails },
        { "Engine frame limit exit reason", testFrameLimitExitReason },
        { "Engine script requested exit reason", testScriptRequestedExitReason },
        { "Engine second run fails", testSecondRunFails },
        { "Engine invalid dimensions fail before window", testInvalidDimensionsFailBeforeWindow },
        { "Engine scale overflow fails before window", testScaleOverflowFailsBeforeWindow },
        { "CLI propagates Engine failure", testCliPropagatesEngineFailure }
    };

    for (const auto& test : tests)
    {
        try
        {
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
