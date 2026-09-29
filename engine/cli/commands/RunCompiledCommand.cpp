#include "RunCompiledCommand.h"

#include "../../compiler/CompiledProjectBinary.h"
#include "../../core/Engine.h"
#include "../DiagnosticPrinter.h"
#include "../CliExitCode.h"

#include <cctype>
#include <filesystem>
#include <iostream>

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
}

int RunCompiledCommand::execute(const CliArguments& arguments) const
{
    if (arguments.format == CliOutputFormat::Json)
    {
        Diagnostics diagnostics;
        diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
            "--format=json is not supported for run-compiled yet");

        DiagnosticPrinter::printDiagnostics(
            diagnostics,
            CliOutputFormat::Text,
            "run-compiled",
            CliExitCode::InvalidArguments,
            false,
            std::cout,
            std::cerr
        );

        return static_cast<int>(CliExitCode::InvalidArguments);
    }

    if (!equalsIgnoreCase(arguments.target.extension().string(), ".flxc"))
    {
        Diagnostics diagnostics;
        diagnostics.error(
            DiagnosticCode::CliErrorUnclassified,
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
    EngineResult engineResult =
        engine.run(result.project, arguments.runOptions);

    DiagnosticPrinter::printDiagnostics(
        engineResult.diagnostics,
        arguments.format,
        "run-compiled",
        engineResult.success ? CliExitCode::Success : CliExitCode::RuntimeInitializationError,
        engineResult.success,
        std::cout,
        std::cerr
    );

    if (!engineResult.success)
    {
        return static_cast<int>(CliExitCode::RuntimeInitializationError);
    }

    return static_cast<int>(CliExitCode::Success);
}
