#pragma once

#include <string>
#include <vector>

enum class DiagnosticSeverity
{
    Info,
    Warning,
    Error
};

struct Diagnostic
{
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string file;
    std::string field;
    std::string message;
};

class Diagnostics
{
public:
    void info(
        const std::string& message,
        const std::string& file = "",
        const std::string& field = ""
    );

    void warning(
        const std::string& message,
        const std::string& file = "",
        const std::string& field = ""
    );

    void error(
        const std::string& message,
        const std::string& file = "",
        const std::string& field = ""
    );

    bool hasErrors() const;
    const std::vector<Diagnostic>& all() const;

private:
    void add(
        DiagnosticSeverity severity,
        const std::string& message,
        const std::string& file,
        const std::string& field
    );

    std::vector<Diagnostic> diagnostics;
};

