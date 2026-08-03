#include "Diagnostics.h"

#include <algorithm>

void Diagnostics::info(
    const std::string& message,
    const std::string& file,
    const std::string& field
)
{
    add(DiagnosticSeverity::Info, message, file, field);
}

void Diagnostics::warning(
    const std::string& message,
    const std::string& file,
    const std::string& field
)
{
    add(DiagnosticSeverity::Warning, message, file, field);
}

void Diagnostics::error(
    const std::string& message,
    const std::string& file,
    const std::string& field
)
{
    add(DiagnosticSeverity::Error, message, file, field);
}

bool Diagnostics::hasErrors() const
{
    return std::any_of(
        diagnostics.begin(),
        diagnostics.end(),
        [](const Diagnostic& diagnostic)
        {
            return diagnostic.severity == DiagnosticSeverity::Error;
        }
    );
}

const std::vector<Diagnostic>& Diagnostics::all() const
{
    return diagnostics;
}

void Diagnostics::add(
    DiagnosticSeverity severity,
    const std::string& message,
    const std::string& file,
    const std::string& field
)
{
    diagnostics.push_back(
        Diagnostic{
            severity,
            file,
            field,
            message
        }
    );
}

