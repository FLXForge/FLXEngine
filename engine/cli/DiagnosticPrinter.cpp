#include "DiagnosticPrinter.h"

#include <nlohmann/json.hpp>

#include <ostream>

namespace
{
    std::string severityName(DiagnosticSeverity severity)
    {
        switch (severity)
        {
        case DiagnosticSeverity::Error:
            return "error";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Info:
        default:
            return "info";
        }
    }

    nlohmann::json diagnosticsToJson(const Diagnostics& diagnostics)
    {
        nlohmann::json items =
            nlohmann::json::array();

        for (const Diagnostic& diagnostic : diagnostics.all())
        {
            nlohmann::json item =
                {
                    { "severity", severityName(diagnostic.severity) },
                    { "code", diagnosticCodeText(diagnostic.code) },
                    { "identifier", diagnostic.identifier },
                    { "file", diagnostic.file },
                    { "field", diagnostic.field },
                    { "message", diagnostic.message }
                };

            if (diagnostic.range.has_value())
            {
                item["range"] =
                    {
                        { "startLine", diagnostic.range->start.line },
                        { "startColumn", diagnostic.range->start.column },
                        { "endLine", diagnostic.range->end.line },
                        { "endColumn", diagnostic.range->end.column }
                    };
            }

            items.push_back(item);
        }

        return items;
    }

    void printTextDiagnostic(
        const Diagnostic& diagnostic,
        std::ostream& stream
    )
    {
        stream
            << severityName(diagnostic.severity) << " "
            << diagnosticCodeText(diagnostic.code) << " "
            << diagnostic.identifier << ": ";

        if (!diagnostic.file.empty())
        {
            stream << diagnostic.file;

            if (!diagnostic.field.empty())
            {
                stream << " [" << diagnostic.field << "]";
            }

            stream << ": ";
        }

        stream << diagnostic.message << "\n";
    }

    // Human diagnostics always go to stderr, including severity "info".
    // Stdout remains reserved for requested results and JSON output.
}

void DiagnosticPrinter::printDiagnostics(
    const Diagnostics& diagnostics,
    CliOutputFormat format,
    const std::string& command,
    CliExitCode exitCode,
    bool success,
    std::ostream& output,
    std::ostream& error
)
{
    if (format == CliOutputFormat::Json)
    {
        nlohmann::json payload =
            {
                { "success", success },
                { "command", command },
                { "exitCode", static_cast<int>(exitCode) },
                { "diagnostics", diagnosticsToJson(diagnostics) }
            };

        output << payload.dump() << "\n";
        return;
    }

    for (const Diagnostic& diagnostic : diagnostics.all())
    {
        printTextDiagnostic(diagnostic, error);
    }
}

void DiagnosticPrinter::printResult(
    const std::string& command,
    const std::string& message,
    CliOutputFormat format,
    std::ostream& output,
    std::ostream& error,
    const Diagnostics& diagnostics
)
{
    if (format == CliOutputFormat::Json)
    {
        nlohmann::json payload =
            {
                { "success", true },
                { "command", command },
                { "exitCode", 0 },
                { "diagnostics", diagnosticsToJson(diagnostics) },
                { "message", message }
            };

        output << payload.dump() << "\n";
        return;
    }

    for (const Diagnostic& diagnostic : diagnostics.all())
    {
        printTextDiagnostic(diagnostic, error);
    }

    output << message << "\n";
}
