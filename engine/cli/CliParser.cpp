#include "CliParser.h"

#include <algorithm>
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

    CliCommand commandFromString(const std::string& value)
    {
        if (value == "compile")
        {
            return CliCommand::Compile;
        }

        if (value == "run-compiled")
        {
            return CliCommand::RunCompiled;
        }

        if (value == "validate")
        {
            return CliCommand::Validate;
        }

        if (value == "version")
        {
            return CliCommand::Version;
        }

        if (value == "help")
        {
            return CliCommand::Help;
        }

        return CliCommand::Run;
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
        if (option == "--frames")
        {
            return command == CliCommand::Run || command == CliCommand::RunCompiled;
        }

        if (option == "--output")
        {
            return command == CliCommand::Compile;
        }

        if (option == "--format")
        {
            return true;
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

    if (first == "--version" || first == "-v")
    {
        result.arguments.command = CliCommand::Version;
        return result;
    }

    if (first == "--help" || first == "-h")
    {
        result.arguments.command = CliCommand::Help;
        return result;
    }

    if (isKnownCommand(first))
    {
        result.arguments.command = commandFromString(first);
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
            "Unknown command: " + first + "\nRun 'flx --help' for usage information."
        );
        return result;
    }

    if (result.arguments.command == CliCommand::Help && index < tokens.size())
    {
        const std::string& helpTarget = tokens[index];

        if (isKnownCommand(helpTarget) && helpTarget != "help" && helpTarget != "version")
        {
            result.arguments.helpCommand = commandFromString(helpTarget);
            ++index;
        }
    }

    bool targetSet =
        explicitTargetSet;

    while (index < tokens.size())
    {
        const std::string token =
            tokens[index];

        if (token == "--help" || token == "-h")
        {
            result.arguments.helpCommand = result.arguments.command;
            result.arguments.command = CliCommand::Help;
            ++index;
            continue;
        }

        if (token == "--version" || token == "-v")
        {
            result.arguments.command = CliCommand::Version;
            ++index;
            continue;
        }

        if (token.rfind("--frames=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--frames"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--frames is not valid for " + commandName(result.arguments.command));
                return result;
            }

            int frames = 0;

            if (!parsePositiveInt(token.substr(9), frames))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--frames must be a positive integer");
                return result;
            }

            result.arguments.maxFrames = frames;
            ++index;
            continue;
        }

        if (token.rfind("--output=", 0) == 0)
        {
            if (!optionAllowed(result.arguments.command, "--output"))
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--output is only valid for compile");
                return result;
            }

            if (result.arguments.output.has_value())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--output cannot be specified more than once");
                return result;
            }

            const std::string output =
                token.substr(9);

            if (output.empty())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--output requires a path");
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
                result.diagnostics.error("--output is only valid for compile");
                return result;
            }

            if (result.arguments.output.has_value())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--output cannot be specified more than once");
                return result;
            }

            if (index + 1 >= tokens.size() || tokens[index + 1].empty())
            {
                result.success = false;
                result.exitCode = CliExitCode::InvalidArguments;
                result.diagnostics.error("--output requires a path");
                return result;
            }

            result.arguments.output = tokens[index + 1];
            index += 2;
            continue;
        }

        if (token.rfind("--format=", 0) == 0)
        {
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
                result.diagnostics.error("--format must be text or json");
                return result;
            }

            ++index;
            continue;
        }

        if (!token.empty() && token[0] == '-')
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error("Unknown option: " + token);
            return result;
        }

        if (targetSet)
        {
            result.success = false;
            result.exitCode = CliExitCode::InvalidArguments;
            result.diagnostics.error("Only one target can be specified");
            return result;
        }

        result.arguments.target = token;
        targetSet = true;
        ++index;
    }

    if (result.arguments.command == CliCommand::Compile && !result.arguments.output.has_value())
    {
        result.success = false;
        result.exitCode = CliExitCode::InvalidArguments;
        result.diagnostics.error("compile requires --output=<path>");
        return result;
    }

    return result;
}
