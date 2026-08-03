#include "RunCompiledCommand.h"

#include "../../compiler/CompiledProjectBinary.h"
#include "../../core/Engine.h"
#include "../DiagnosticPrinter.h"
#include "../CliExitCode.h"

#include <filesystem>
#include <iostream>

int RunCompiledCommand::execute(const CliArguments& arguments) const
{
    if (arguments.target.extension() != ".flxc")
    {
        Diagnostics diagnostics;
        diagnostics.error(
            "run-compiled requires a .flxc target",
            arguments.target.generic_string()
        );

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            arguments.format,
            "run-compiled",
            CliExitCode::InvalidArguments,
            false,
            std::cout,
            std::cerr
        );

        return static_cast<int>(CliExitCode::InvalidArguments);
    }

    CompiledProjectBinaryResult result =
        CompiledProjectReader::read(arguments.target.generic_string());

    DiagnosticPrinter::printDiagnostics(
        result.diagnostics,
        arguments.format,
        "run-compiled",
        result.success ? CliExitCode::Success : CliExitCode::InvalidCompiledProject,
        result.success,
        std::cout,
        std::cerr
    );

    if (!result.success)
    {
        return static_cast<int>(CliExitCode::InvalidCompiledProject);
    }

    Engine engine;
    engine.run(
        result.project,
        arguments.maxFrames.value_or(-1)
    );

    return static_cast<int>(CliExitCode::Success);
}
