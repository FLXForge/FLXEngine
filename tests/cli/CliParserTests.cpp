#include "../support/TestSupport.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

    void testCliParserDefaults()
    {
        CliParseResult result =
            parseArguments({});

        require(result.success, "empty CLI should parse");
        require(result.arguments.command == CliCommand::Run, "empty CLI should default to run");
        require(result.arguments.target == ".", "empty CLI should default target to current directory");

        result =
            parseArguments({ "." });

        require(result.success, "directory target should parse");
        require(result.arguments.command == CliCommand::Run, "directory target should imply run");
        require(result.arguments.target == ".", "directory target should be preserved");

        result =
            parseArguments({ "project.flx" });

        require(result.success, ".flx target should parse");
        require(result.arguments.command == CliCommand::Run, ".flx target should imply run");
        require(result.arguments.target == "project.flx", ".flx target should be preserved");

        result =
            parseArguments({ "run" });

        require(result.success, "run without target should parse");
        require(result.arguments.command == CliCommand::Run, "run command should parse");
        require(result.arguments.target == ".", "run should default target to current directory");
    }

    void testCliParserCommands()
    {
        CliParseResult result =
            parseArguments({ "run", "--frames=10", "." });

        require(result.success, "run with frames should parse");
        require(result.arguments.command == CliCommand::Run, "run command should be selected");
        require(result.arguments.maxFrames == 10, "frames should parse");

        result =
            parseArguments({ "compile", "--output=game.flxc", "." });

        require(result.success, "compile with output should parse");
        require(result.arguments.command == CliCommand::Compile, "compile command should be selected");
        require(result.arguments.output->generic_string() == "game.flxc", "compile output should parse");

        result =
            parseArguments({ "compile", "-o", "game.flxc", "." });

        require(result.success, "compile with -o should parse");
        require(result.arguments.output->generic_string() == "game.flxc", "-o output should parse");

        result =
            parseArguments({ "run-compiled", "game.flxc" });

        require(result.success, "run-compiled should parse");
        require(result.arguments.command == CliCommand::RunCompiled, "run-compiled command should be selected");
        require(result.arguments.target == "game.flxc", "run-compiled target should parse");

        result =
            parseArguments({ "validate", "." });

        require(result.success, "validate should parse");
        require(result.arguments.command == CliCommand::Validate, "validate command should be selected");

        result =
            parseArguments({ "validate", "--format=json", "." });

        require(result.success, "validate should accept json format");
        require(result.arguments.format == CliOutputFormat::Json, "validate json format should parse");

        result =
            parseArguments({ "compile", "--format=json", "--output=game.flxc", "." });

        require(result.success, "compile should accept json format");
        require(result.arguments.format == CliOutputFormat::Json, "compile json format should parse");
    }

    void testCliParserHelpAndVersion()
    {
        CliParseResult result =
            parseArguments({ "--version" });

        require(result.success, "--version should parse");
        require(result.arguments.command == CliCommand::Version, "--version should select version");

        result =
            parseArguments({ "-v" });

        require(result.success, "-v should parse");
        require(result.arguments.command == CliCommand::Version, "-v should select version");

        result =
            parseArguments({ "version" });

        require(result.success, "version command should parse");
        require(result.arguments.command == CliCommand::Version, "version command should be selected");

        result =
            parseArguments({ "--help" });

        require(result.success, "--help should parse");
        require(result.arguments.command == CliCommand::Help, "--help should select help");

        result =
            parseArguments({ "help", "run" });

        require(result.success, "help run should parse");
        require(result.arguments.command == CliCommand::Help, "help command should be selected");
        require(result.arguments.helpCommand == CliCommand::Run, "help run should select run help");

        result =
            parseArguments({ "help", "version" });

        require(result.success, "help version should parse");
        require(result.arguments.command == CliCommand::Help, "help version should select help");
        require(result.arguments.helpCommand == CliCommand::Version, "help version should select version help");

        result =
            parseArguments({ "run", "--help" });

        require(result.success, "run --help should parse");
        require(result.arguments.command == CliCommand::Help, "run --help should select help");
        require(result.arguments.helpCommand == CliCommand::Run, "run --help should select run help");

        result =
            parseArguments({ "version", "--help" });

        require(result.success, "version --help should parse");
        require(result.arguments.command == CliCommand::Help, "version --help should select help");
        require(result.arguments.helpCommand == CliCommand::Version, "version --help should select version help");
    }

    void testCliParserInvalidArguments()
    {
        CliParseResult result =
            parseArguments({ "unknown" });

        require(!result.success, "unknown command should fail");
        require(result.exitCode == CliExitCode::InvalidArguments, "unknown command should return invalid arguments");

        result =
            parseArguments({ "run", "--frames=abc", "." });

        require(!result.success, "non numeric frames should fail");

        result =
            parseArguments({ "run", "--frames=0", "." });

        require(!result.success, "zero frames should fail");

        result =
            parseArguments({ "run", "--frames=-1", "." });

        require(!result.success, "negative frames should fail");

        result =
            parseArguments({ "compile", "." });

        require(!result.success, "compile without output should fail");

        result =
            parseArguments({ "compile", "--output=a.flxc", "--output=b.flxc", "." });

        require(!result.success, "duplicate output should fail");

        result =
            parseArguments({ "--version", "." });

        require(!result.success, "--version with extra arguments should fail");

        result =
            parseArguments({ "-v", "--format=json" });

        require(!result.success, "-v with extra arguments should fail");

        result =
            parseArguments({ "version", "." });

        require(!result.success, "version with target should fail");

        result =
            parseArguments({ "version", "--format=json" });

        require(!result.success, "version with format should fail");

        result =
            parseArguments({ "--help", "run" });

        require(!result.success, "--help with trailing arguments should fail");

        result =
            parseArguments({ "-h", "run" });

        require(!result.success, "-h with trailing arguments should fail");

        result =
            parseArguments({ "help", "missing" });

        require(!result.success, "help with unknown command should fail");

        result =
            parseArguments({ "help", "run", "extra" });

        require(!result.success, "help with extra arguments should fail");

        result =
            parseArguments({ "compile", "-o", "--format=json", "." });

        require(!result.success, "-o should not consume another option as output path");

        result =
            parseArguments({ "run", "--frames=10", "--frames=20", "." });

        require(!result.success, "duplicate frames should fail");

        result =
            parseArguments({ "validate", "--format=text", "--format=json", "." });

        require(!result.success, "duplicate format should fail");

        result =
            parseArguments({ "run", "--version" });

        require(!result.success, "run --version should fail instead of changing command");

        result =
            parseArguments({ "run", ".", "--version" });

        require(!result.success, "run target --version should fail instead of changing command");

        result =
            parseArguments({ "run", "--format=json", "." });

        require(!result.success, "run should reject json format");

        result =
            parseArguments({ "run-compiled", "--format=json", "game.flxc" });

        require(!result.success, "run-compiled should reject json format");

        result =
            parseArguments({ "help", "--format=json" });

        require(!result.success, "help should reject format");

        result =
            parseArguments({ "--format=json" });

        require(!result.success, "default run should reject json format");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "CLI parser defaults", testCliParserDefaults },

        { "CLI parser commands", testCliParserCommands },

        { "CLI parser help and version", testCliParserHelpAndVersion },

        { "CLI parser invalid arguments", testCliParserInvalidArguments }

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

