#include "DiagnosticPrinter.h"

#include <ostream>
#include <sstream>
#include <vector>

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

    std::string escapeJson(const std::string& text)
    {
        std::ostringstream output;

        for (const char character : text)
        {
            switch (character)
            {
            case '\\':
                output << "\\\\";
                break;
            case '"':
                output << "\\\"";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                output << character;
                break;
            }
        }

        return output.str();
    }

    void printTextDiagnostic(
        const Diagnostic& diagnostic,
        std::ostream& stream
    )
    {
        stream << severityName(diagnostic.severity) << ": ";

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

    void printJsonDiagnostics(
        const Diagnostics& diagnostics,
        std::ostream& output
    )
    {
        output << "\"diagnostics\":[";

        const std::vector<Diagnostic>& all =
            diagnostics.all();

        for (std::size_t i = 0; i < all.size(); ++i)
        {
            const Diagnostic& diagnostic =
                all[i];

            if (i > 0)
            {
                output << ",";
            }

            output
                << "{"
                << "\"severity\":\"" << severityName(diagnostic.severity) << "\","
                << "\"file\":\"" << escapeJson(diagnostic.file) << "\","
                << "\"field\":\"" << escapeJson(diagnostic.field) << "\","
                << "\"message\":\"" << escapeJson(diagnostic.message) << "\""
                << "}";
        }

        output << "]";
    }
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
        output
            << "{"
            << "\"success\":" << (success ? "true" : "false") << ","
            << "\"command\":\"" << escapeJson(command) << "\","
            << "\"exitCode\":" << static_cast<int>(exitCode) << ",";
        printJsonDiagnostics(diagnostics, output);
        output << "}\n";
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
    const Diagnostics& diagnostics
)
{
    if (format == CliOutputFormat::Json)
    {
        output
            << "{"
            << "\"success\":true,"
            << "\"command\":\"" << escapeJson(command) << "\","
            << "\"exitCode\":0,"
            << "\"message\":\"" << escapeJson(message) << "\",";
        printJsonDiagnostics(diagnostics, output);
        output << "}\n";
        return;
    }

    output << message << "\n";
}
