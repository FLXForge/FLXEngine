#include "RunCommand.h"

#include "../../compiler/ProjectCompiler.h"
#include "../../core/Engine.h"
#include "../DiagnosticPrinter.h"
#include "../ProjectResolver.h"

#include <iostream>

int RunCommand::execute(const CliArguments& arguments) const
{
    if (arguments.format == CliOutputFormat::Json)
    {
        Diagnostics diagnostics;
        diagnostics.error("--format=json is not supported for run yet");

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            CliOutputFormat::Text,
            "run",
            CliExitCode::InvalidArguments,
            false,
            std::cout,
            std::cerr
        );

        return static_cast<int>(CliExitCode::InvalidArguments);
    }

    ProjectResolver resolver;
    ProjectResolutionResult resolution =
        resolver.resolve(arguments.target);

    if (!resolution.success)
    {
        DiagnosticPrinter::printDiagnostics(
            resolution.diagnostics,
            arguments.format,
            "run",
            resolution.exitCode,
            false,
            std::cout,
            std::cerr
        );

        return static_cast<int>(resolution.exitCode);
    }

    ProjectCompiler compiler;
    CompilationResult result =
        compiler.compile(resolution.manifestPath.generic_string());

    DiagnosticPrinter::printDiagnostics(
        result.diagnostics,
        arguments.format,
        "run",
        result.success ? CliExitCode::Success : CliExitCode::CompilationError,
        result.success,
        std::cout,
        std::cerr
    );

    if (!result.success)
    {
        return static_cast<int>(CliExitCode::CompilationError);
    }

    Engine engine;
    engine.run(
        result.project,
        arguments.maxFrames.value_or(-1)
    );

    return static_cast<int>(CliExitCode::Success);
}
