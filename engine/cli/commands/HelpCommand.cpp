#include "HelpCommand.h"

#include "../CliExitCode.h"

#include <iostream>

namespace
{
    void printGeneralHelp()
    {
        std::cout
            << "Usage:\n"
            << "  flx [command] [options] [target]\n"
            << "\n"
            << "Commands:\n"
            << "  run             Compile and run a project\n"
            << "  compile         Generate a compiled FLX project\n"
            << "  run-compiled    Run a compiled FLX project\n"
            << "  validate        Validate a project\n"
            << "  version         Show FLX version\n"
            << "  help            Show help\n"
            << "\n"
            << "Options:\n"
            << "  --help, -h\n"
            << "  --version, -v\n"
            << "  --frames=<number>              Only for run and run-compiled\n"
            << "  --window-mode=<window|fullscreen>\n"
            << "                                 Only for run and run-compiled\n"
            << "  --scale=<number>               Only for run and run-compiled\n"
            << "  --debug-logs[=<true|false>]    Only for run and run-compiled\n"
            << "  --debug-console[=<true|false>] Only for run and run-compiled\n"
            << "  --debug-collisions[=<true|false>]\n"
            << "                                 Only for run and run-compiled\n"
            << "  --output=<path>                Only for compile\n"
            << "  --format=<text|json>    Only for compile and validate\n"
            << "\n"
            << "Examples:\n"
            << "  flx\n"
            << "  flx run --frames=120 --scale=3 .\n"
            << "  flx compile --output=game.flxc .\n"
            << "  flx run-compiled --window-mode=fullscreen game.flxc\n"
            << "\n"
            << "Use 'flx help <command>' for command-specific help.\n";
    }

    void printRunHelp()
    {
        std::cout
            << "Usage:\n"
            << "  flx run [options] [target]\n"
            << "\n"
            << "Options:\n"
            << "  --frames=<number>              Limit runtime frames\n"
            << "  --window-mode=<window|fullscreen>\n"
            << "                                 Default: window\n"
            << "  --scale=<number>               Override machine output scale for this run\n"
            << "  --debug-logs[=<true|false>]    Enable or disable runtime debug logs\n"
            << "  --debug-console[=<true|false>] Enable or disable console log output\n"
            << "  --debug-collisions[=<true|false>]\n"
            << "                                 Draw collision debug overlay\n"
            << "\n"
            << "Boolean runtime options default to false. When the value is omitted, "
            << "the option means true.\n"
            << "\n"
            << "JSON runtime output is pending until Engine returns a RuntimeResult "
            << "and can control its streams.\n";
    }

    void printCompileHelp()
    {
        std::cout
            << "Usage:\n"
            << "  flx compile --output=<path> [target]\n"
            << "\n"
            << "Options:\n"
            << "  --output=<path>      Compiled .flxc output path\n"
            << "  -o <path>            Alias for --output\n"
            << "  --format=<text|json>\n";
    }

    void printRunCompiledHelp()
    {
        std::cout
            << "Usage:\n"
            << "  flx run-compiled [options] target.flxc\n"
            << "\n"
            << "Options:\n"
            << "  --frames=<number>              Limit runtime frames\n"
            << "  --window-mode=<window|fullscreen>\n"
            << "                                 Default: window\n"
            << "  --scale=<number>               Override machine output scale for this run\n"
            << "  --debug-logs[=<true|false>]    Enable or disable runtime debug logs\n"
            << "  --debug-console[=<true|false>] Enable or disable console log output\n"
            << "  --debug-collisions[=<true|false>]\n"
            << "                                 Draw collision debug overlay\n"
            << "\n"
            << "Boolean runtime options default to false. When the value is omitted, "
            << "the option means true.\n"
            << "\n"
            << "JSON runtime output is pending until Engine returns a RuntimeResult "
            << "and can control its streams.\n";
    }

    void printValidateHelp()
    {
        std::cout
            << "Usage:\n"
            << "  flx validate [options] [target]\n"
            << "\n"
            << "Options:\n"
            << "  --format=<text|json>\n";
    }

    void printVersionHelp()
    {
        std::cout
            << "Usage:\n"
            << "  flx version\n"
            << "  flx --version\n"
            << "  flx -v\n";
    }
}

int HelpCommand::execute(const CliArguments& arguments) const
{
    if (!arguments.helpCommand.has_value())
    {
        printGeneralHelp();
        return static_cast<int>(CliExitCode::Success);
    }

    switch (*arguments.helpCommand)
    {
    case CliCommand::Run:
        printRunHelp();
        break;
    case CliCommand::Compile:
        printCompileHelp();
        break;
    case CliCommand::RunCompiled:
        printRunCompiledHelp();
        break;
    case CliCommand::Validate:
        printValidateHelp();
        break;
    case CliCommand::Version:
        printVersionHelp();
        break;
    default:
        printGeneralHelp();
        break;
    }

    return static_cast<int>(CliExitCode::Success);
}
