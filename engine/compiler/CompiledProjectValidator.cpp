#include "CompiledProjectValidator.h"

bool CompiledProjectValidator::validate(
    const CompiledProject& project,
    Diagnostics& diagnostics,
    const std::string& source
)
{
    if (project.rootId.empty())
    {
        diagnostics.error(
            DiagnosticCode::CompiledProjectMissingRoot,
            "Compiled project root id is empty",
            source,
            "rootId"
        );
    }
    else if (project.resources.findObject(project.rootId) == nullptr)
    {
        diagnostics.error(
            DiagnosticCode::CompiledProjectMissingRoot,
            "Compiled project root resource is missing",
            source,
            "rootId"
        );
    }

    for (const auto& objectPair : project.resources.allObjects())
    {
        const ResourceId& objectId =
            objectPair.first;
        const ObjectDefinition& object =
            objectPair.second;

        if (
            object.id.find('#') != std::string::npos &&
            object.id != objectId
            )
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectIdentityMismatch,
                "Compiled object registry key does not match object id",
                source,
                objectId
            );
        }

        if (!object.children.empty())
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectEmbeddedChildren,
                "Compiled object still contains embedded children",
                source,
                objectId
            );
        }

        for (const auto& childPair : object.childResources)
        {
            if (project.resources.findObject(childPair.second) == nullptr)
            {
                diagnostics.error(
                    DiagnosticCode::CompiledProjectMissingChildResource,
                    "Compiled child resource points to a missing object",
                    source,
                    objectId + ".childResources." + childPair.first
                );
            }
        }

        for (const std::string& scriptId : object.resolvedScriptPaths)
        {
            if (project.resources.findScript(scriptId) == nullptr)
            {
                diagnostics.error(
                    DiagnosticCode::CompiledProjectMissingScriptResource,
                    "Compiled object script points to a missing script",
                    source,
                    objectId + ".resolvedScriptPaths"
                );
            }
        }
    }

    for (const auto& scriptPair : project.resources.allScripts())
    {
        const ResourceId& scriptId =
            scriptPair.first;
        const ScriptResource& script =
            scriptPair.second;

        if (script.id != scriptId)
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectIdentityMismatch,
                "Compiled script registry key does not match script id",
                source,
                scriptId
            );
        }
    }

    return !diagnostics.hasErrors();
}
