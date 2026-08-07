#include "Diagnostics.h"

#include <algorithm>
#include <cstddef>

namespace
{
    const std::vector<DiagnosticCodeDefinition>& definitions()
    {
        static const std::vector<DiagnosticCodeDefinition> items = {
            { DiagnosticCode::CliInformationUnclassified, "FLX-CLI-00000", "CliInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::CLI, "Unclassified informational CLI diagnostic." },
            { DiagnosticCode::CliWarningUnclassified, "FLX-CLI-00001", "CliWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::CLI, "Unclassified CLI warning." },
            { DiagnosticCode::CliErrorUnclassified, "FLX-CLI-00002", "CliErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::CLI, "Unclassified CLI error." },
            { DiagnosticCode::CliUnexpectedInternalError, "FLX-CLI-00003", "CliUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::CLI, "Unexpected internal CLI error." },
            { DiagnosticCode::CliDeprecatedFunctionality, "FLX-CLI-00004", "CliDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::CLI, "Deprecated CLI functionality." },

            { DiagnosticCode::ProjectInformationUnclassified, "FLX-PROJECT-00000", "ProjectInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::PROJECT, "Unclassified informational project diagnostic." },
            { DiagnosticCode::ProjectWarningUnclassified, "FLX-PROJECT-00001", "ProjectWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::PROJECT, "Unclassified project warning." },
            { DiagnosticCode::ProjectErrorUnclassified, "FLX-PROJECT-00002", "ProjectErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Unclassified project error." },
            { DiagnosticCode::ProjectUnexpectedInternalError, "FLX-PROJECT-00003", "ProjectUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Unexpected internal project error." },
            { DiagnosticCode::ProjectDeprecatedFunctionality, "FLX-PROJECT-00004", "ProjectDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::PROJECT, "Deprecated project functionality." },
            { DiagnosticCode::ProjectManifestCouldNotBeOpened, "FLX-PROJECT-00010", "ProjectManifestCouldNotBeOpened", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Project manifest could not be opened." },
            { DiagnosticCode::InvalidProjectManifestSyntax, "FLX-PROJECT-00011", "InvalidProjectManifestSyntax", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Project manifest line has invalid syntax." },
            { DiagnosticCode::UnknownProjectManifestField, "FLX-PROJECT-00012", "UnknownProjectManifestField", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Project manifest contains an unknown field." },
            { DiagnosticCode::DuplicateProjectManifestField, "FLX-PROJECT-00013", "DuplicateProjectManifestField", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Project manifest contains a duplicate field." },
            { DiagnosticCode::MissingProjectRoot, "FLX-PROJECT-00014", "MissingProjectRoot", DiagnosticSeverity::Error, DiagnosticDomain::PROJECT, "Project manifest root field is missing or empty." },

            { DiagnosticCode::CompInformationUnclassified, "FLX-COMP-00000", "CompInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::COMP, "Unclassified informational compiler diagnostic." },
            { DiagnosticCode::CompWarningUnclassified, "FLX-COMP-00001", "CompWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::COMP, "Unclassified compiler warning." },
            { DiagnosticCode::CompErrorUnclassified, "FLX-COMP-00002", "CompErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Unclassified compiler error." },
            { DiagnosticCode::CompUnexpectedInternalError, "FLX-COMP-00003", "CompUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Unexpected internal compiler error." },
            { DiagnosticCode::CompDeprecatedFunctionality, "FLX-COMP-00004", "CompDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::COMP, "Deprecated compiler functionality." },
            { DiagnosticCode::CompiledProjectMissingRoot, "FLX-COMP-00010", "CompiledProjectMissingRoot", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Compiled project root id or root resource is missing." },
            { DiagnosticCode::CompiledProjectEmbeddedChildren, "FLX-COMP-00011", "CompiledProjectEmbeddedChildren", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Compiled project object still contains embedded children." },
            { DiagnosticCode::CompiledProjectMissingChildResource, "FLX-COMP-00012", "CompiledProjectMissingChildResource", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Compiled project child resource points to a missing object." },
            { DiagnosticCode::CompiledProjectMissingScriptResource, "FLX-COMP-00013", "CompiledProjectMissingScriptResource", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Compiled project script reference points to a missing script." },
            { DiagnosticCode::CompiledProjectIdentityMismatch, "FLX-COMP-00014", "CompiledProjectIdentityMismatch", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Compiled project registry key does not match the stored resource id." },
            { DiagnosticCode::AutomaticInstantiationCycle, "FLX-COMP-00015", "AutomaticInstantiationCycle", DiagnosticSeverity::Error, DiagnosticDomain::COMP, "Compiled project contains an automatic instantiation cycle." },

            { DiagnosticCode::ResourceInformationUnclassified, "FLX-RESOURCE-00000", "ResourceInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::RESOURCE, "Unclassified informational resource diagnostic." },
            { DiagnosticCode::ResourceWarningUnclassified, "FLX-RESOURCE-00001", "ResourceWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::RESOURCE, "Unclassified resource warning." },
            { DiagnosticCode::ResourceErrorUnclassified, "FLX-RESOURCE-00002", "ResourceErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Unclassified resource error." },
            { DiagnosticCode::ResourceUnexpectedInternalError, "FLX-RESOURCE-00003", "ResourceUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Unexpected internal resource error." },
            { DiagnosticCode::ResourceDeprecatedFunctionality, "FLX-RESOURCE-00004", "ResourceDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::RESOURCE, "Deprecated resource functionality." },
            { DiagnosticCode::MissingReferencedResource, "FLX-RESOURCE-00010", "MissingReferencedResource", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Referenced resource could not be found." },
            { DiagnosticCode::MissingInternalResourceMember, "FLX-RESOURCE-00011", "MissingInternalResourceMember", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Referenced internal resource member could not be found." },
            { DiagnosticCode::ResourceReferenceCycle, "FLX-RESOURCE-00012", "ResourceReferenceCycle", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Resource reference cycle detected." },
            { DiagnosticCode::ResourceIdCollision, "FLX-RESOURCE-00013", "ResourceIdCollision", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Resource id collision detected." },
            { DiagnosticCode::ReferencedScriptNotFound, "FLX-RESOURCE-00014", "ReferencedScriptNotFound", DiagnosticSeverity::Error, DiagnosticDomain::RESOURCE, "Referenced script could not be found." },

            { DiagnosticCode::MachineInformationUnclassified, "FLX-MACHINE-00000", "MachineInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::MACHINE, "Unclassified informational machine diagnostic." },
            { DiagnosticCode::MachineWarningUnclassified, "FLX-MACHINE-00001", "MachineWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::MACHINE, "Unclassified machine warning." },
            { DiagnosticCode::MachineErrorUnclassified, "FLX-MACHINE-00002", "MachineErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::MACHINE, "Unclassified machine error." },
            { DiagnosticCode::MachineUnexpectedInternalError, "FLX-MACHINE-00003", "MachineUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::MACHINE, "Unexpected internal machine error." },
            { DiagnosticCode::MachineDeprecatedFunctionality, "FLX-MACHINE-00004", "MachineDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::MACHINE, "Deprecated machine functionality." },

            { DiagnosticCode::BinaryInformationUnclassified, "FLX-BINARY-00000", "BinaryInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::BINARY, "Unclassified informational binary diagnostic." },
            { DiagnosticCode::BinaryWarningUnclassified, "FLX-BINARY-00001", "BinaryWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::BINARY, "Unclassified binary warning." },
            { DiagnosticCode::BinaryErrorUnclassified, "FLX-BINARY-00002", "BinaryErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Unclassified binary error." },
            { DiagnosticCode::BinaryUnexpectedInternalError, "FLX-BINARY-00003", "BinaryUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Unexpected internal binary error." },
            { DiagnosticCode::BinaryDeprecatedFunctionality, "FLX-BINARY-00004", "BinaryDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::BINARY, "Deprecated binary functionality." },
            { DiagnosticCode::CompiledProjectCouldNotBeOpened, "FLX-BINARY-00010", "CompiledProjectCouldNotBeOpened", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project file could not be opened." },
            { DiagnosticCode::CompiledProjectCouldNotBeCreated, "FLX-BINARY-00011", "CompiledProjectCouldNotBeCreated", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project file could not be created." },
            { DiagnosticCode::CompiledProjectWriteFailed, "FLX-BINARY-00012", "CompiledProjectWriteFailed", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project file could not be written." },
            { DiagnosticCode::InvalidCompiledProjectMagic, "FLX-BINARY-00013", "InvalidCompiledProjectMagic", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project file has invalid magic." },
            { DiagnosticCode::UnsupportedCompiledProjectFormat, "FLX-BINARY-00014", "UnsupportedCompiledProjectFormat", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project format version is not supported." },
            { DiagnosticCode::TruncatedCompiledProject, "FLX-BINARY-00015", "TruncatedCompiledProject", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project file is truncated." },
            { DiagnosticCode::CompiledProjectLimitExceeded, "FLX-BINARY-00016", "CompiledProjectLimitExceeded", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project file exceeds a binary format limit." },
            { DiagnosticCode::DuplicateCompiledResource, "FLX-BINARY-00017", "DuplicateCompiledResource", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project contains a duplicate resource." },
            { DiagnosticCode::TrailingCompiledProjectData, "FLX-BINARY-00018", "TrailingCompiledProjectData", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project contains trailing data." },
            { DiagnosticCode::DuplicateCompiledEntry, "FLX-BINARY-00019", "DuplicateCompiledEntry", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project contains a duplicate entry inside a resource." },
            { DiagnosticCode::InvalidCompiledProjectValue, "FLX-BINARY-00020", "InvalidCompiledProjectValue", DiagnosticSeverity::Error, DiagnosticDomain::BINARY, "Compiled project contains an invalid binary value." },

            { DiagnosticCode::RuntimeInformationUnclassified, "FLX-RUNTIME-00000", "RuntimeInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::RUNTIME, "Unclassified informational runtime diagnostic." },
            { DiagnosticCode::RuntimeWarningUnclassified, "FLX-RUNTIME-00001", "RuntimeWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::RUNTIME, "Unclassified runtime warning." },
            { DiagnosticCode::RuntimeErrorUnclassified, "FLX-RUNTIME-00002", "RuntimeErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Unclassified runtime error." },
            { DiagnosticCode::RuntimeUnexpectedInternalError, "FLX-RUNTIME-00003", "RuntimeUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Unexpected internal runtime error." },
            { DiagnosticCode::RuntimeDeprecatedFunctionality, "FLX-RUNTIME-00004", "RuntimeDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::RUNTIME, "Deprecated runtime functionality." },
            { DiagnosticCode::EngineAlreadyRun, "FLX-RUNTIME-00010", "EngineAlreadyRun", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Engine instance was asked to run more than once." },
            { DiagnosticCode::InvalidVideoOutputConfiguration, "FLX-RUNTIME-00011", "InvalidVideoOutputConfiguration", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Video output configuration is invalid." },
            { DiagnosticCode::WindowInitializationFailed, "FLX-RUNTIME-00012", "WindowInitializationFailed", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Window could not be initialized." },
            { DiagnosticCode::RenderTargetInitializationFailed, "FLX-RUNTIME-00013", "RenderTargetInitializationFailed", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Render target could not be initialized." },
            { DiagnosticCode::RuntimeWorldLoadFailed, "FLX-RUNTIME-00014", "RuntimeWorldLoadFailed", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Runtime world could not be loaded." },
            { DiagnosticCode::RuntimeLoadSpawnLimitExceeded, "FLX-RUNTIME-00015", "RuntimeLoadSpawnLimitExceeded", DiagnosticSeverity::Error, DiagnosticDomain::RUNTIME, "Runtime load spawn limit was exceeded." },

            { DiagnosticCode::BuildInformationUnclassified, "FLX-BUILD-00000", "BuildInformationUnclassified", DiagnosticSeverity::Info, DiagnosticDomain::BUILD, "Unclassified informational build diagnostic." },
            { DiagnosticCode::BuildWarningUnclassified, "FLX-BUILD-00001", "BuildWarningUnclassified", DiagnosticSeverity::Warning, DiagnosticDomain::BUILD, "Unclassified build warning." },
            { DiagnosticCode::BuildErrorUnclassified, "FLX-BUILD-00002", "BuildErrorUnclassified", DiagnosticSeverity::Error, DiagnosticDomain::BUILD, "Unclassified build error." },
            { DiagnosticCode::BuildUnexpectedInternalError, "FLX-BUILD-00003", "BuildUnexpectedInternalError", DiagnosticSeverity::Error, DiagnosticDomain::BUILD, "Unexpected internal build error." },
            { DiagnosticCode::BuildDeprecatedFunctionality, "FLX-BUILD-00004", "BuildDeprecatedFunctionality", DiagnosticSeverity::Warning, DiagnosticDomain::BUILD, "Deprecated build functionality." }
        };

        return items;
    }

    const DiagnosticCodeDefinition& findDefinition(DiagnosticCode code)
    {
        const auto& items =
            definitions();

        const auto it =
            std::find_if(
                items.begin(),
                items.end(),
                [code](const DiagnosticCodeDefinition& definition)
                {
                    return definition.code == code;
                }
            );

        if (it != items.end())
        {
            return *it;
        }

        return items.front();
    }
}

const char* diagnosticCodeText(DiagnosticCode code)
{
    return findDefinition(code).text;
}

const char* diagnosticIdentifier(DiagnosticCode code)
{
    return findDefinition(code).identifier;
}

DiagnosticSeverity diagnosticUsualSeverity(DiagnosticCode code)
{
    return findDefinition(code).usualSeverity;
}

DiagnosticDomain diagnosticDomain(DiagnosticCode code)
{
    return findDefinition(code).domain;
}

const std::vector<DiagnosticCodeDefinition>& registeredDiagnosticCodes()
{
    return definitions();
}

void Diagnostics::info(
    DiagnosticCode code,
    const std::string& message,
    const std::string& file,
    const std::string& field,
    std::optional<SourceRange> range
)
{
    add(DiagnosticSeverity::Info, code, message, file, field, range);
}

void Diagnostics::warning(
    DiagnosticCode code,
    const std::string& message,
    const std::string& file,
    const std::string& field,
    std::optional<SourceRange> range
)
{
    add(DiagnosticSeverity::Warning, code, message, file, field, range);
}

void Diagnostics::error(
    DiagnosticCode code,
    const std::string& message,
    const std::string& file,
    const std::string& field,
    std::optional<SourceRange> range
)
{
    add(DiagnosticSeverity::Error, code, message, file, field, range);
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

bool Diagnostics::empty() const
{
    return diagnostics.empty();
}

std::size_t Diagnostics::size() const
{
    return diagnostics.size();
}

void Diagnostics::append(const Diagnostics& other)
{
    diagnostics.insert(
        diagnostics.end(),
        other.diagnostics.begin(),
        other.diagnostics.end()
    );
}

const std::vector<Diagnostic>& Diagnostics::all() const
{
    return diagnostics;
}

void Diagnostics::add(
    DiagnosticSeverity severity,
    DiagnosticCode code,
    const std::string& message,
    const std::string& file,
    const std::string& field,
    std::optional<SourceRange> range
)
{
    diagnostics.push_back(
        Diagnostic{
            severity,
            code,
            diagnosticIdentifier(code),
            file,
            field,
            message,
            range
        }
    );
}
