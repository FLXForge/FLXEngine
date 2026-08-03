#include "HelpCommand.h"

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
            << "  --frames=<number>\n"
            << "  --output=<path>\n"
            << "  --format=<text|json>\n"
            << "\n"
            << "Examples:\n"
            << "  flx\n"
            << "  flx run --frames=120 .\n"
            << "  flx compile --output=game.flxc .\n"
            << "  flx run-compiled --frames=120 game.flxc\n"
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
            << "  --frames=<number>    Limit runtime frames\n"
            << "  --format=<text|json>\n";
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
            << "  --frames=<number>    Limit runtime frames\n"
            << "  --format=<text|json>\n";
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
}

int HelpCommand::execute(const CliArguments& arguments) const
{
    if (!arguments.helpCommand.has_value())
    {
        printGeneralHelp();
        return 0;
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
    default:
        printGeneralHelp();
        break;
    }

    return 0;
}
