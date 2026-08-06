#include "CompiledProjectValidator.h"

bool CompiledProjectValidator::validate(
    const CompiledProject& project,
    Diagnostics& diagnostics,
    DiagnosticCode code,
    const std::string& source
)
{
    if (project.rootId.empty())
    {
        diagnostics.error(
            code,
            "Compiled project root id is empty",
            source,
            "rootId"
        );
    }
    else if (project.resources.findObject(project.rootId) == nullptr)
    {
        diagnostics.error(
            code,
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

        if (!object.children.empty())
        {
            diagnostics.error(
                code,
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
                    code,
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
                    code,
                    "Compiled object script points to a missing script",
                    source,
                    objectId + ".resolvedScriptPaths"
                );
            }
        }
    }

    return !diagnostics.hasErrors();
}
