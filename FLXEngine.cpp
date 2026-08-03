#include "engine/core/Engine.h"
#include "engine/compiler/CompiledProjectBinary.h"
#include "engine/compiler/ProjectCompiler.h"
#include "engine/debug/Logger.h"

#include <iostream>
#include <string>

inline constexpr const char* FLXENGINE_VERSION = "0.2.0";

namespace
{
    void printDiagnostics(
        const Diagnostics& diagnostics,
        const std::string& channel
    )
    {
        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            const std::string location =
                diagnostic.file.empty()
                ? ""
                : diagnostic.file + (
                    diagnostic.field.empty()
                    ? ""
                    : " [" + diagnostic.field + "]"
                ) + ": ";

            if (diagnostic.severity == DiagnosticSeverity::Error)
            {
                Logger::error(channel, location + diagnostic.message);
            }
            else if (diagnostic.severity == DiagnosticSeverity::Warning)
            {
                Logger::warning(channel, location + diagnostic.message);
            }
            else
            {
                Logger::debug(channel, location + diagnostic.message);
            }
        }
    }

    int parseMaxFrames(int argc, char* argv[])
    {
        int maxFrames =
            -1;

        constexpr const char* framesPrefix =
            "--frames=";

        for (int i = 1; i < argc; ++i)
        {
            const std::string option =
                argv[i];

            if (option.rfind(framesPrefix, 0) == 0)
            {
                maxFrames =
                    std::stoi(option.substr(std::string(framesPrefix).size()));
            }
        }

        return maxFrames;
    }

    std::string outputPath(int argc, char* argv[])
    {
        for (int i = 1; i + 1 < argc; ++i)
        {
            if (std::string(argv[i]) == "-o")
            {
                return argv[i + 1];
            }
        }

        return "";
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger::error("run", "Usage: FlxEngine.exe [run] project.flx | compile project.flx -o project.flxc | run-compiled project.flxc");
        return 1;
    }

    Logger::info("engine", std::string("FLXEngine version ") + FLXENGINE_VERSION);

    const std::string command =
        argv[1];

    const int maxFrames =
        parseMaxFrames(argc, argv);

    if (command == "compile")
    {
        if (argc < 4)
        {
            Logger::error("compiler", "Usage: FlxEngine.exe compile project.flx -o project.flxc");
            return 1;
        }

        const std::string output =
            outputPath(argc, argv);

        if (output.empty())
        {
            Logger::error("compiler", "Missing output path. Use -o project.flxc");
            return 1;
        }

        ProjectCompiler compiler;
        CompilationResult result =
            compiler.compile(argv[2]);

        Logger::setConsoleEnabled(true);

        printDiagnostics(
            result.diagnostics,
            "compiler"
        );

        if (!result.success)
        {
            if (!result.diagnostics.hasErrors())
            {
                Logger::error("compiler", "Compilation failed without diagnostics");
            }

            return 1;
        }

        Diagnostics writeDiagnostics;

        if (!CompiledProjectWriter::write(
            output,
            result.project,
            writeDiagnostics
        ))
        {
            Logger::setConsoleEnabled(true);

            printDiagnostics(
                writeDiagnostics,
                "compiler"
            );

            return 1;
        }

        Logger::info("compiler", "Compiled project written: " + output);
        return 0;
    }

    if (command == "run-compiled")
    {
        if (argc < 3)
        {
            Logger::error("run", "Usage: FlxEngine.exe run-compiled project.flxc");
            return 1;
        }

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(argv[2]);

        Logger::setConsoleEnabled(true);

        printDiagnostics(
            result.diagnostics,
            "compiler"
        );

        if (!result.success)
        {
            if (!result.diagnostics.hasErrors())
            {
                Logger::error("compiler", "Compiled project could not be loaded");
            }

            return 1;
        }

        Engine engine;
        engine.run(result.project, maxFrames);
        return 0;
    }
    else
    {
        const char* projectPath =
            command == "run" && argc >= 3
            ? argv[2]
            : argv[1];

        ProjectCompiler compiler;
        CompilationResult result =
            compiler.compile(projectPath);

        Logger::setConsoleEnabled(true);

        printDiagnostics(
            result.diagnostics,
            "compiler"
        );

        if (!result.success)
        {
            if (!result.diagnostics.hasErrors())
            {
                Logger::error("compiler", "Compilation failed without diagnostics");
            }

            return 1;
        }

        Engine engine;
        engine.run(result.project, maxFrames);
        return 0;
    }
}
