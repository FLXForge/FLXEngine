#include "ValidateCommand.h"

#include "../../compiler/ProjectCompiler.h"
#include "../DiagnosticPrinter.h"
#include "../ProjectResolver.h"

#include <iostream>

int ValidateCommand::execute(const CliArguments& arguments) const
{
    ProjectResolver resolver;
    ProjectResolutionResult resolution =
        resolver.resolve(arguments.target);

    if (!resolution.success)
    {
        DiagnosticPrinter::printDiagnostics(
            resolution.diagnostics,
            arguments.format,
            "validate",
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

    if (!result.success)
    {
        DiagnosticPrinter::printDiagnostics(
            result.diagnostics,
            arguments.format,
            "validate",
            result.success ? CliExitCode::Success : CliExitCode::CompilationError,
            result.success,
            std::cout,
            std::cerr
        );
    }

    if (!result.success)
    {
        return static_cast<int>(CliExitCode::CompilationError);
    }

    DiagnosticPrinter::printResult(
        "validate",
        "Project is valid: " + resolution.manifestPath.generic_string(),
        arguments.format,
        std::cout,
        std::cerr,
        result.diagnostics
    );

    return static_cast<int>(CliExitCode::Success);
}
