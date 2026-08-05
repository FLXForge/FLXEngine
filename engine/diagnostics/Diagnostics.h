#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

enum class DiagnosticSeverity
{
    Info,
    Warning,
    Error
};

enum class DiagnosticDomain
{
    CLI,
    PROJECT,
    COMP,
    RESOURCE,
    MACHINE,
    BINARY,
    RUNTIME,
    BUILD
};

enum class DiagnosticCode
{
    CliInformationUnclassified,
    CliWarningUnclassified,
    CliErrorUnclassified,
    CliUnexpectedInternalError,
    CliDeprecatedFunctionality,

    ProjectInformationUnclassified,
    ProjectWarningUnclassified,
    ProjectErrorUnclassified,
    ProjectUnexpectedInternalError,
    ProjectDeprecatedFunctionality,
    ProjectManifestCouldNotBeOpened,
    InvalidProjectManifestSyntax,
    UnknownProjectManifestField,
    DuplicateProjectManifestField,
    MissingProjectRoot,

    CompInformationUnclassified,
    CompWarningUnclassified,
    CompErrorUnclassified,
    CompUnexpectedInternalError,
    CompDeprecatedFunctionality,

    ResourceInformationUnclassified,
    ResourceWarningUnclassified,
    ResourceErrorUnclassified,
    ResourceUnexpectedInternalError,
    ResourceDeprecatedFunctionality,
    MissingReferencedResource,
    MissingInternalResourceMember,
    ResourceReferenceCycle,
    ResourceIdCollision,
    ReferencedScriptNotFound,

    MachineInformationUnclassified,
    MachineWarningUnclassified,
    MachineErrorUnclassified,
    MachineUnexpectedInternalError,
    MachineDeprecatedFunctionality,

    BinaryInformationUnclassified,
    BinaryWarningUnclassified,
    BinaryErrorUnclassified,
    BinaryUnexpectedInternalError,
    BinaryDeprecatedFunctionality,

    RuntimeInformationUnclassified,
    RuntimeWarningUnclassified,
    RuntimeErrorUnclassified,
    RuntimeUnexpectedInternalError,
    RuntimeDeprecatedFunctionality,

    BuildInformationUnclassified,
    BuildWarningUnclassified,
    BuildErrorUnclassified,
    BuildUnexpectedInternalError,
    BuildDeprecatedFunctionality
};

struct SourcePosition
{
    int line = 1;
    int column = 1;
};

struct SourceRange
{
    SourcePosition start;
    SourcePosition end;
};

struct Diagnostic
{
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    DiagnosticCode code = DiagnosticCode::CompInformationUnclassified;
    std::string identifier;
    std::string file;
    std::string field;
    std::string message;
    std::optional<SourceRange> range;
};

struct DiagnosticCodeDefinition
{
    DiagnosticCode code;
    const char* text;
    const char* identifier;
    DiagnosticSeverity usualSeverity;
    DiagnosticDomain domain;
    const char* description;
};

const char* diagnosticCodeText(DiagnosticCode code);
const char* diagnosticIdentifier(DiagnosticCode code);
DiagnosticSeverity diagnosticUsualSeverity(DiagnosticCode code);
DiagnosticDomain diagnosticDomain(DiagnosticCode code);
const std::vector<DiagnosticCodeDefinition>& registeredDiagnosticCodes();

class Diagnostics
{
public:
    void info(
        DiagnosticCode code,
        const std::string& message,
        const std::string& file = "",
        const std::string& field = "",
        std::optional<SourceRange> range = std::nullopt
    );

    void warning(
        DiagnosticCode code,
        const std::string& message,
        const std::string& file = "",
        const std::string& field = "",
        std::optional<SourceRange> range = std::nullopt
    );

    void error(
        DiagnosticCode code,
        const std::string& message,
        const std::string& file = "",
        const std::string& field = "",
        std::optional<SourceRange> range = std::nullopt
    );

    bool hasErrors() const;
    bool empty() const;
    std::size_t size() const;
    void append(const Diagnostics& other);
    const std::vector<Diagnostic>& all() const;

private:
    void add(
        DiagnosticSeverity severity,
        DiagnosticCode code,
        const std::string& message,
        const std::string& file,
        const std::string& field,
        std::optional<SourceRange> range
    );

    std::vector<Diagnostic> diagnostics;
};
