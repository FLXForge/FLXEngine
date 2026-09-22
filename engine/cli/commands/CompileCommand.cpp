#include "CompileCommand.h"

#include "../../compiler/CompiledProjectBinary.h"
#include "../../compiler/ProjectCompiler.h"
#include "../DiagnosticPrinter.h"
#include "../ProjectResolver.h"

#include <iostream>

int CompileCommand::execute(const CliArguments& arguments) const
{
    if (!arguments.output.has_value())
    {
        Diagnostics diagnostics;
        diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "compile requires --output=<path>");

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            arguments.format,
            "compile",
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

    if (!result.success)
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

    const bool writeSuccess =
        CompiledProjectWriter::write(
        output,
        result.project,
        writeDiagnostics
    );

    Diagnostics diagnostics;
    diagnostics.append(result.diagnostics);
    diagnostics.append(writeDiagnostics);

    if (!writeSuccess)
    {
        DiagnosticPrinter::printDiagnostics(
            diagnostics,
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
        std::cerr,
        diagnostics
    );

    return static_cast<int>(CliExitCode::Success);
}
