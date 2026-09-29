#include "CliApplication.h"

#include "CliParser.h"
#include "DiagnosticPrinter.h"
#include "commands/CompileCommand.h"
#include "commands/HelpCommand.h"
#include "commands/RunCommand.h"
#include "commands/RunCompiledCommand.h"
#include "commands/ValidateCommand.h"
#include "commands/VersionCommand.h"

#include <iostream>

namespace
{
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
}

int CliApplication::run(int argc, char* argv[]) const
{
    CliParser parser;
    CliParseResult parsed =
        parser.parse(argc, argv);

    if (!parsed.success)
    {
        DiagnosticPrinter::printDiagnostics(
            parsed.diagnostics,
            parsed.arguments.format,
            commandName(parsed.arguments.command),
            parsed.exitCode,
            false,
            std::cout,
            std::cerr
        );

        return static_cast<int>(parsed.exitCode);
    }

    switch (parsed.arguments.command)
    {
    case CliCommand::Compile:
        return CompileCommand().execute(parsed.arguments);
    case CliCommand::RunCompiled:
        return RunCompiledCommand().execute(parsed.arguments);
    case CliCommand::Validate:
        return ValidateCommand().execute(parsed.arguments);
    case CliCommand::Version:
        return VersionCommand().execute();
    case CliCommand::Help:
        return HelpCommand().execute(parsed.arguments);
    case CliCommand::Run:
    default:
        return RunCommand().execute(parsed.arguments);
    }
}
