#include "CliParser.h"

#include <charconv>
#include <cctype>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    bool equalsIgnoreCase(
        const std::string& left,
        const std::string& right
    )
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < left.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(left[i])) !=
                std::tolower(static_cast<unsigned char>(right[i])))
            {
                return false;
            }
        }

        return true;
    }

    bool endsWithFlx(const std::string& value)
    {
        return value.size() >= 4 &&
            equalsIgnoreCase(value.substr(value.size() - 4), ".flx");
    }

    bool pathExists(const std::string& value)
    {
        std::error_code error;
        return std::filesystem::exists(value, error);
    }

    bool parsePositiveInt(
        const std::string& text,
        int& value
    )
    {
        if (text.empty())
        {
            return false;
        }

        int parsed = 0;
        const char* begin = text.data();
        const char* end = text.data() + text.size();
        const std::from_chars_result result =
            std::from_chars(begin, end, parsed);

        if (result.ec != std::errc() || result.ptr != end || parsed <= 0)
        {
            return false;
        }

        value = parsed;
        return true;
    }

    bool parseBoolean(
        const std::string& text,
        bool& value
    )
    {
        if (text == "true")
        {
            value = true;
            return true;
        }

        if (text == "false")
        {
            value = false;
            return true;
        }

        return false;
    }

    bool isKnownCommand(const std::string& value)
    {
        return
            value == "run" ||
            value == "compile" ||
            value == "run-compiled" ||
            value == "validate" ||
            value == "version" ||
            value == "help";
    }

    bool commandFromString(
        const std::string& value,
        CliCommand& command
    )
    {
        if (value == "run")
        {
            command = CliCommand::Run;
            return true;
        }

        if (value == "compile")
        {
            command = CliCommand::Compile;
            return true;
        }

        if (value == "run-compiled")
        {
            command = CliCommand::RunCompiled;
            return true;
        }

        if (value == "validate")
        {
            command = CliCommand::Validate;
            return true;
        }

        if (value == "version")
        {
            command = CliCommand::Version;
            return true;
        }

        if (value == "help")
        {
            command = CliCommand::Help;
            return true;
        }

        return false;
    }

    std::string commandName(CliCommand command)
    {
        switch (command)
        {
        case CliCommand::Compile:
            return "compile";
        case CliCommand::RunCompiled:
            return "run-compiled";
        case CliCommand::Validate:
            return "validate";
        case CliCommand::Version:
            return "version";
        case CliCommand::Help:
            return "help";
        case CliCommand::Run:
        default:
            return "run";
        }
    }

    bool optionAllowed(
        CliCommand command,
        const std::string& option
    )
    {
        if (
            option == "--frames" ||
            option == "--window-mode" ||
            option == "--scale" ||
            option == "--debug-logs" ||
            option == "--debug-console" ||
            option == "--debug-collisions"
        )
        {
            return command == CliCommand::Run || command == CliCommand::RunCompiled;
        }

        if (option == "--output")
        {
            return command == CliCommand::Compile;
        }

        if (option == "--format")
        {
            return command == CliCommand::Compile || command == CliCommand::Validate;
        }

        return false;
    }
}

CliParseResult CliParser::parse(int argc, char* argv[]) const
{
    CliParseResult result;
    result.success = true;
    result.exitCode = CliExitCode::Success;

    std::vector<std::string> tokens;

    for (int i = 1; i < argc; ++i)
    {
        tokens.emplace_back(argv[i]);
    }

    if (tokens.empty())
    {
        result.arguments.command = CliCommand::Run;
        result.arguments.target = ".";
        return result;
    }

    std::size_t index = 0;
    const std::string& first = tokens[index];
    bool explicitTargetSet = false;
    bool formatSet = false;
    bool windowModeSet = false;
    bool scaleSet = false;
    bool debugLogsSet = false;
    bool debugConsoleSet = false;
    bool debugCollisionsSet = false;

    if (first == "--version" || first == "-v")
    {
        result.arguments.command = CliCommand::Version;
        if (tokens.size() > 1)
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "--version does not accept additional arguments");
        }

        return result;
    }

    if (first == "--help" || first == "-h")
    {
        result.arguments.command = CliCommand::Help;
        if (tokens.size() > 1)
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "--help does not accept additional arguments. Use 'flx help <command>'.");
        }

        return result;
    }

    if (isKnownCommand(first))
    {
        if (!commandFromString(first, result.arguments.command))
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "Unknown command: " + first);
            return result;
        }

        ++index;
    }
    else if (
        pathExists(first) ||
        endsWithFlx(first)
    )
    {
        result.arguments.command = CliCommand::Run;
        result.arguments.target = first;
        explicitTargetSet = true;
        ++index;
    }
    else if (!first.empty() && first[0] != '-')
    {
        result.success = false;
        result.exitCode = CliExitCode::InvalidArguments;
        result.diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "Unknown command: " + first + "\nRun 'flx --help' for usage information."
        );
        return result;
    }

    if (result.arguments.command == CliCommand::Help && index < tokens.size())
    {
        const std::string& helpTarget = tokens[index];

        if (!isKnownCommand(helpTarget))
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "Unknown command for help: " + helpTarget + "\nRun 'flx --help' for usage information."
            );
            return result;
        }

        if (helpTarget == "help")
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "help does not provide command-specific help for itself");
            return result;
        }

        CliCommand helpCommand = CliCommand::Run;

        if (!commandFromString(helpTarget, helpCommand))
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "Unknown command for help: " + helpTarget);
            return result;
        }

        result.arguments.helpCommand = helpCommand;
        ++index;

        if (index < tokens.size())
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "help accepts only one optional command");
            return result;
        }
    }

    bool targetSet =
        explicitTargetSet;

    while (index < tokens.size())
    {
        const std::string token =
            tokens[index];

        if (
            result.arguments.command == CliCommand::Version &&
            token != "--help" &&
            token != "-h"
        )
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "version does not accept options or targets");
            return result;
        }

        if (token == "--help" || token == "-h")
        {
            if (index + 1 < tokens.size())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--help does not accept trailing arguments");
                return result;
            }

            const CliCommand commandToDescribe =
                result.arguments.command;

            result.arguments.command = CliCommand::Help;
            result.arguments.helpCommand = commandToDescribe;
            ++index;
            continue;
        }

        if (token == "--version" || token == "-v")
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "--version is only valid without additional arguments");
            return result;
        }

        if (token.rfind("--frames=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--frames"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--frames is not valid for " + commandName(result.arguments.command));
                return result;
            }

            int frames = 0;

            if (result.arguments.runOptions.maxFrames.has_value())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--frames cannot be specified more than once");
                return result;
            }

            if (!parsePositiveInt(token.substr(9), frames))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--frames must be a positive integer");
                return result;
            }

            result.arguments.runOptions.maxFrames = frames;
            ++index;
            continue;
        }

        if (token.rfind("--window-mode=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--window-mode"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--window-mode is only valid for run and run-compiled");
                return result;
            }

            if (windowModeSet)
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--window-mode cannot be specified more than once");
                return result;
            }

            const std::string mode =
                token.substr(14);

            if (mode == "window")
            {
                result.arguments.runOptions.windowMode =
                    WindowMode::Window;
            }
            else if (mode == "fullscreen")
            {
                result.arguments.runOptions.windowMode =
                    WindowMode::Fullscreen;
            }
            else
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--window-mode must be window or fullscreen");
                return result;
            }

            windowModeSet = true;
            ++index;
            continue;
        }

        if (token.rfind("--scale=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--scale"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--scale is only valid for run and run-compiled");
                return result;
            }

            if (scaleSet)
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--scale cannot be specified more than once");
                return result;
            }

            int scale = 0;

            if (!parsePositiveInt(token.substr(8), scale))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--scale must be a positive integer");
                return result;
            }

            result.arguments.runOptions.scaleOverride = scale;
            scaleSet = true;
            ++index;
            continue;
        }

        if (
            token == "--debug-logs" ||
            token.rfind("--debug-logs=", 0) == 0
        )
        {
            if (!optionAllowed(result.arguments.command, "--debug-logs"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-logs is only valid for run and run-compiled");
                return result;
            }

            if (debugLogsSet)
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-logs cannot be specified more than once");
                return result;
            }

            bool enabled = true;

            if (token.rfind("--debug-logs=", 0) == 0 &&
                !parseBoolean(token.substr(13), enabled))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-logs must be true or false");
                return result;
            }

            result.arguments.runOptions.debugLogs = enabled;
            debugLogsSet = true;
            ++index;
            continue;
        }

        if (
            token == "--debug-console" ||
            token.rfind("--debug-console=", 0) == 0
        )
        {
            if (!optionAllowed(result.arguments.command, "--debug-console"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-console is only valid for run and run-compiled");
                return result;
            }

            if (debugConsoleSet)
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-console cannot be specified more than once");
                return result;
            }

            bool enabled = true;

            if (token.rfind("--debug-console=", 0) == 0 &&
                !parseBoolean(token.substr(16), enabled))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-console must be true or false");
                return result;
            }

            result.arguments.runOptions.debugConsole = enabled;
            debugConsoleSet = true;
            ++index;
            continue;
        }

        if (
            token == "--debug-collisions" ||
            token.rfind("--debug-collisions=", 0) == 0
        )
        {
            if (!optionAllowed(result.arguments.command, "--debug-collisions"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-collisions is only valid for run and run-compiled");
                return result;
            }

            if (debugCollisionsSet)
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-collisions cannot be specified more than once");
                return result;
            }

            bool enabled = true;

            if (token.rfind("--debug-collisions=", 0) == 0 &&
                !parseBoolean(token.substr(19), enabled))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--debug-collisions must be true or false");
                return result;
            }

            result.arguments.runOptions.debugCollisions = enabled;
            ++index;
            debugCollisionsSet = true;
            continue;
        }

        if (token.rfind("--output=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--output"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--output is only valid for compile");
                return result;
            }

            if (result.arguments.output.has_value())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--output cannot be specified more than once");
                return result;
            }

            const std::string output =
                token.substr(9);

            if (output.empty())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--output requires a path");
                return result;
            }

            result.arguments.output = output;
            ++index;
            continue;
        }

        if (token == "--output" || token == "-o")
        {
            if (!optionAllowed(result.arguments.command, "--output"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--output is only valid for compile");
                return result;
            }

            if (result.arguments.output.has_value())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--output cannot be specified more than once");
                return result;
            }

            if (
                index + 1 >= tokens.size() ||
                tokens[index + 1].empty() ||
                tokens[index + 1][0] == '-'
            )
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--output requires a path");
                return result;
            }

            result.arguments.output = tokens[index + 1];
            index += 2;
            continue;
        }

        if (token.rfind("--format=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--format"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--format is only valid for compile and validate");
                return result;
            }

            if (formatSet)
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--format cannot be specified more than once");
                return result;
            }

            const std::string format =
                token.substr(9);

            if (format == "text")
            {
                result.arguments.format = CliOutputFormat::Text;
            }
            else if (format == "json")
            {
                result.arguments.format = CliOutputFormat::Json;
            }
            else
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error(
                    DiagnosticCode::CliErrorUnclassified,
                    "--format must be text or json");
                return result;
            }

            formatSet = true;
            ++index;
            continue;
        }

        if (!token.empty() && token[0] == '-')
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "Unknown option: " + token);
            return result;
        }

        if (targetSet)
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error(
                DiagnosticCode::CliErrorUnclassified,
                "Only one target can be specified");
            return result;
        }

        result.arguments.target = token;
        targetSet = true;
        ++index;
    }

    if (result.arguments.command == CliCommand::Version && targetSet)
    {
        result.success = false;
        result.exitCode = CliExitCode::InvalidArguments;
        result.diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "version does not accept a target");
        return result;
    }

    if (result.arguments.command == CliCommand::Compile && !result.arguments.output.has_value())
    {
        result.success = false;
        result.exitCode = CliExitCode::InvalidArguments;
        result.diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "compile requires --output=<path>");
        return result;
    }

    return result;
}
