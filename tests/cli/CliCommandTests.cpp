#include "../support/TestSupport.h"
#include "../../engine/cli/commands/CompileCommand.h"
#include "../../engine/cli/commands/HelpCommand.h"
#include "../../engine/cli/commands/RunCommand.h"
#include "../../engine/cli/commands/RunCompiledCommand.h"
#include "../../engine/cli/commands/ValidateCommand.h"
#include "../../engine/cli/commands/VersionCommand.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

    void testValidateCommandSuccessOutputContract()
    {
        CliArguments arguments;
        arguments.command = CliCommand::Validate;
        arguments.target = createMinimalProject("validate_command_success");

        StreamCapture capture;
        const int exitCode =
            ValidateCommand().execute(arguments);

        require(exitCode == static_cast<int>(CliExitCode::Success), "validate command should succeed");
        require(
            capture.output.str().find("Project is valid: ") == 0,
            "validate success message should be written to stdout"
        );
        require(
            capture.error.str().find("info FLX-COMP-00000 CompInformationUnclassified: ") != std::string::npos,
            "validate success diagnostics should be written to stderr"
        );
        require(
            countOccurrences(capture.error.str(), "Project compiled") == 1,
            "validate success diagnostics should not be duplicated"
        );
    }

    void testCompileCommandSuccessOutputContract()
    {
        const std::filesystem::path manifest =
            createMinimalProject("compile_command_success");

        const std::filesystem::path output =
            testRoot() / "compile_command_success" / "game.flxc";

        CliArguments arguments;
        arguments.command = CliCommand::Compile;
        arguments.target = manifest;
        arguments.output = output;

        StreamCapture capture;
        const int exitCode =
            CompileCommand().execute(arguments);

        require(exitCode == static_cast<int>(CliExitCode::Success), "compile command should succeed");
        require(
            capture.output.str() == "Compiled project written: " + output.generic_string() + "\n",
            "compile success message should be written to stdout"
        );
        require(
            countOccurrences(capture.error.str(), "Project compiled") == 1,
            "compile success diagnostics should not be duplicated"
        );
        require(std::filesystem::exists(output), "compile command should write compiled output");
    }

    void testCompileCommandMissingOutputDirect()
    {
        CliArguments arguments;
        arguments.command = CliCommand::Compile;
        arguments.target = createMinimalProject("compile_missing_output");

        StreamCapture capture;
        const int exitCode =
            CompileCommand().execute(arguments);

        require(
            exitCode == static_cast<int>(CliExitCode::InvalidArguments),
            "compile command without output should return invalid arguments"
        );
        require(capture.output.str().empty(), "compile missing output should not write stdout");
        require(
            capture.error.str().find("compile requires --output=<path>") != std::string::npos,
            "compile missing output should report clear error"
        );
    }

    void testValidateAndCompileJsonOutput()
    {
        CliArguments validateArguments;
        validateArguments.command = CliCommand::Validate;
        validateArguments.target = createMinimalProject("validate_json_output");
        validateArguments.format = CliOutputFormat::Json;

        {
            StreamCapture capture;
            const int exitCode =
                ValidateCommand().execute(validateArguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "validate json should succeed");
            require(capture.error.str().empty(), "validate json should keep stderr empty");

            const nlohmann::json payload =
                nlohmann::json::parse(capture.output.str());

            require(payload["success"] == true, "validate json should report success");
            require(payload["command"] == "validate", "validate json should include command");
            require(payload["message"].get<std::string>().find("Project is valid: ") == 0, "validate json should include message");
        }

        const std::filesystem::path manifest =
            createMinimalProject("compile_json_output");
        const std::filesystem::path output =
            testRoot() / "compile_json_output" / "game.flxc";

        CliArguments compileArguments;
        compileArguments.command = CliCommand::Compile;
        compileArguments.target = manifest;
        compileArguments.output = output;
        compileArguments.format = CliOutputFormat::Json;

        {
            StreamCapture capture;
            const int exitCode =
                CompileCommand().execute(compileArguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "compile json should succeed");
            require(capture.error.str().empty(), "compile json should keep stderr empty");

            const nlohmann::json payload =
                nlohmann::json::parse(capture.output.str());

            require(payload["success"] == true, "compile json should report success");
            require(payload["command"] == "compile", "compile json should include command");
            require(payload["message"] == "Compiled project written: " + output.generic_string(), "compile json should include message");
        }
    }

    void testRunCommandsRejectJsonFormat()
    {
        CliArguments runArguments;
        runArguments.command = CliCommand::Run;
        runArguments.format = CliOutputFormat::Json;

        {
            StreamCapture capture;
            const int exitCode =
                RunCommand().execute(runArguments);

            require(exitCode == static_cast<int>(CliExitCode::InvalidArguments), "run command should reject json format");
            require(capture.output.str().empty(), "run json rejection should keep stdout empty");
            require(capture.error.str().find("--format=json is not supported for run yet") != std::string::npos, "run json rejection should explain error");
        }

        CliArguments runCompiledArguments;
        runCompiledArguments.command = CliCommand::RunCompiled;
        runCompiledArguments.target = "game.flxc";
        runCompiledArguments.format = CliOutputFormat::Json;

        {
            StreamCapture capture;
            const int exitCode =
                RunCompiledCommand().execute(runCompiledArguments);

            require(exitCode == static_cast<int>(CliExitCode::InvalidArguments), "run-compiled command should reject json format");
            require(capture.output.str().empty(), "run-compiled json rejection should keep stdout empty");
            require(capture.error.str().find("--format=json is not supported for run-compiled yet") != std::string::npos, "run-compiled json rejection should explain error");
        }
    }

    void testHelpVersionAndVersionOutput()
    {
        CliArguments helpArguments;
        helpArguments.command = CliCommand::Help;
        helpArguments.helpCommand = CliCommand::Version;

        {
            StreamCapture capture;
            const int exitCode =
                HelpCommand().execute(helpArguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "help version should succeed");
            require(capture.error.str().empty(), "help version should not write stderr");
            require(capture.output.str().find("flx version") != std::string::npos, "help version should describe version command");
            require(capture.output.str().find("--format") == std::string::npos, "help version should not announce format");
        }

        const std::string expectedVersion =
            readFirstLine(std::filesystem::path(FLX_SOURCE_DIR) / "VERSION");

        {
            StreamCapture capture;
            const int exitCode =
                VersionCommand().execute();

            require(exitCode == static_cast<int>(CliExitCode::Success), "version command should succeed");
            require(capture.error.str().empty(), "version command should not write stderr");
            require(capture.output.str() == expectedVersion + "\n", "version command should write exact version");
        }
    }

    void testRuntimeOptionsHelpContract()
    {
        {
            CliArguments arguments;
            arguments.command = CliCommand::Help;

            StreamCapture capture;
            const int exitCode =
                HelpCommand().execute(arguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "general help should succeed");
            require(capture.output.str().find("--window-mode=<window|fullscreen>") != std::string::npos, "general help should announce window mode");
            require(capture.output.str().find("--scale=<number>") != std::string::npos, "general help should announce scale");
            require(capture.output.str().find("--debug-collisions[=<true|false>]") != std::string::npos, "general help should announce debug collisions");
            require(capture.output.str().find("embedded") == std::string::npos, "help should not announce embedded mode");
        }

        {
            CliArguments arguments;
            arguments.command = CliCommand::Help;
            arguments.helpCommand = CliCommand::Run;

            StreamCapture capture;
            const int exitCode =
                HelpCommand().execute(arguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "run help should succeed");
            require(capture.output.str().find("--window-mode=<window|fullscreen>") != std::string::npos, "run help should announce window mode");
            require(capture.output.str().find("--scale=<number>") != std::string::npos, "run help should announce scale");
            require(capture.output.str().find("--format") == std::string::npos, "run help should not announce format");
        }

        {
            CliArguments arguments;
            arguments.command = CliCommand::Help;
            arguments.helpCommand = CliCommand::RunCompiled;

            StreamCapture capture;
            const int exitCode =
                HelpCommand().execute(arguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "run-compiled help should succeed");
            require(capture.output.str().find("--debug-logs[=<true|false>]") != std::string::npos, "run-compiled help should announce debug logs");
            require(capture.output.str().find("--format") == std::string::npos, "run-compiled help should not announce format");
        }

        {
            CliArguments arguments;
            arguments.command = CliCommand::Help;
            arguments.helpCommand = CliCommand::Compile;

            StreamCapture capture;
            const int exitCode =
                HelpCommand().execute(arguments);

            require(exitCode == static_cast<int>(CliExitCode::Success), "compile help should succeed");
            require(capture.output.str().find("--window-mode") == std::string::npos, "compile help should not announce runtime options");
        }
    }

    void testRunCompiledExtensionAndCommandExitCodes()
    {
        const std::filesystem::path upperCompiled =
            testRoot() / "run_compiled_extension" / "GAME.FLXC";

        writeBinary(
            upperCompiled,
            { 'F', 'L', 'X', 'C', 99, 0, 0, 0 }
        );

        CliArguments runCompiledArguments;
        runCompiledArguments.command = CliCommand::RunCompiled;
        runCompiledArguments.target = upperCompiled;

        {
            StreamCapture capture;
            const int exitCode =
                RunCompiledCommand().execute(runCompiledArguments);

            require(
                exitCode == static_cast<int>(CliExitCode::InvalidCompiledProject),
                "run-compiled should accept .FLXC and then validate file content"
            );
            require(capture.output.str().empty(), "invalid compiled project should keep stdout empty");
            require(!capture.error.str().empty(), "invalid compiled project should write diagnostics");
        }

        runCompiledArguments.target =
            testRoot() / "run_compiled_extension" / "game.txt";

        {
            StreamCapture capture;
            const int exitCode =
                RunCompiledCommand().execute(runCompiledArguments);

            require(
                exitCode == static_cast<int>(CliExitCode::InvalidArguments),
                "run-compiled should reject non .flxc extension"
            );
        }

        CliArguments validateArguments;
        validateArguments.command = CliCommand::Validate;
        validateArguments.target = testRoot() / "missing_validate_project";

        {
            StreamCapture capture;
            const int exitCode =
                ValidateCommand().execute(validateArguments);

            require(
                exitCode == static_cast<int>(CliExitCode::ProjectResolutionError),
                "validate resolution failure should preserve exit code"
            );
        }

        CliArguments compileArguments;
        compileArguments.command = CliCommand::Compile;
        compileArguments.target = testRoot() / "missing_compile_project";
        compileArguments.output = testRoot() / "missing_compile_project.flxc";

        {
            StreamCapture capture;
            const int exitCode =
                CompileCommand().execute(compileArguments);

            require(
                exitCode == static_cast<int>(CliExitCode::ProjectResolutionError),
                "compile resolution failure should preserve exit code"
            );
        }

        CliArguments runArguments;
        runArguments.command = CliCommand::Run;
        runArguments.target = testRoot() / "missing_run_project";

        {
            StreamCapture capture;
            const int exitCode =
                RunCommand().execute(runArguments);

            require(
                exitCode == static_cast<int>(CliExitCode::ProjectResolutionError),
                "run resolution failure should preserve exit code"
            );
        }
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "ValidateCommand success output contract", testValidateCommandSuccessOutputContract },

        { "CompileCommand success output contract", testCompileCommandSuccessOutputContract },

        { "CompileCommand missing output direct", testCompileCommandMissingOutputDirect },

        { "Validate and compile JSON output", testValidateAndCompileJsonOutput },

        { "Run commands reject JSON format", testRunCommandsRejectJsonFormat },

        { "Help version and version output", testHelpVersionAndVersionOutput },

        { "Runtime options help contract", testRuntimeOptionsHelpContract },

        { "RunCompiled extension and command exit codes", testRunCompiledExtensionAndCommandExitCodes }

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

