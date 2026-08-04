#pragma once

#include "CliArguments.h"
#include "CliExitCode.h"
#include "../diagnostics/Diagnostics.h"

#include <iosfwd>
#include <string>

class DiagnosticPrinter
{
public:
    static void printDiagnostics(
        const Diagnostics& diagnostics,
        CliOutputFormat format,
        const std::string& command,
        CliExitCode exitCode,
        bool success,
        std::ostream& output,
        std::ostream& error
    );

    static void printResult(
        const std::string& command,
        const std::string& message,
        CliOutputFormat format,
        std::ostream& output,
        std::ostream& error,
        const Diagnostics& diagnostics
    );
};
