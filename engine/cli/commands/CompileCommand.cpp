#include "CompileCommand.h"

#include "../../compiler/CompiledProjectBinary.h"
#include "../../compiler/ProjectCompiler.h"
#include "../DiagnosticPrinter.h"
#include "../ProjectResolver.h"

#include <iostream>

int CompileCommand::execute(const CliArguments& arguments) const
{
    ProjectResolver resolver;
    ProjectResolutionResult resolution =
        resolver.resolve(arguments.target);

    if (!resolution.success)
    {
        DiagnosticPrinter::printDiagnostics(
            resolution.diagnostics,
            arguments.format,
            "compile",
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

    if (arguments.format == CliOutputFormat::Text || !result.success)
    {
        DiagnosticPrinter::printDiagnostics(
            result.diagnostics,
            arguments.format,
            "compile",
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

    Diagnostics writeDiagnostics;
    const std::string output =
        arguments.output->generic_string();

    if (!CompiledProjectWriter::write(
        output,
        result.project,
        writeDiagnostics
    ))
    {
        DiagnosticPrinter::printDiagnostics(
            writeDiagnostics,
            arguments.format,
            "compile",
            CliExitCode::CompilationError,
            false,
            std::cout,
            std::cerr
        );

        return static_cast<int>(CliExitCode::CompilationError);
    }

    DiagnosticPrinter::printResult(
        "compile",
        "Compiled project written: " + output,
        arguments.format,
        std::cout,
        result.diagnostics
    );

    return static_cast<int>(CliExitCode::Success);
}
