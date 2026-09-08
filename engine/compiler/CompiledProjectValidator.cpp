#include "CompiledProjectValidator.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace
{
    bool detectAutomaticCycleFrom(
        const ResourceId& objectId,
        const CompiledProject& project,
        std::vector<ResourceId>& stack,
        std::unordered_set<ResourceId>& completed,
        Diagnostics& diagnostics,
        const std::string& source
    )
    {
        if (completed.find(objectId) != completed.end())
        {
            return false;
        }

        const auto stackIt =
            std::find(
                stack.begin(),
                stack.end(),
                objectId
            );

        if (stackIt != stack.end())
        {
            std::string chain;

            for (auto it = stackIt; it != stack.end(); ++it)
            {
                if (!chain.empty())
                {
                    chain += " -> ";
                }

                chain += *it;
            }

            chain += " -> " + objectId;

            diagnostics.error(
                DiagnosticCode::AutomaticInstantiationCycle,
                "Automatic instantiation cycle detected: " + chain,
                source,
                objectId
            );

            return true;
        }

        const ObjectDefinition* object =
            project.resources.findObject(objectId);

        if (object == nullptr)
        {
            return false;
        }

        stack.push_back(objectId);

        bool foundCycle = false;

        for (const auto& childPair : object->childResources)
        {
            const ObjectDefinition* child =
                project.resources.findObject(childPair.second);

            if (child == nullptr || child->spawnMode != "auto")
            {
                continue;
            }

            foundCycle =
                detectAutomaticCycleFrom(
                    childPair.second,
                    project,
                    stack,
                    completed,
                    diagnostics,
                    source
                ) || foundCycle;
        }

        stack.pop_back();
        completed.insert(objectId);

        return foundCycle;
    }

    void validateAutomaticInstantiationCycles(
        const CompiledProject& project,
        Diagnostics& diagnostics,
        const std::string& source
    )
    {
        std::unordered_set<ResourceId> completed;

        for (const auto& objectPair : project.resources.allObjects())
        {
            std::vector<ResourceId> stack;

            detectAutomaticCycleFrom(
                objectPair.first,
                project,
                stack,
                completed,
                diagnostics,
                source
            );
        }
    }

    void validateStateMachine(
        const ObjectDefinition& object,
        Diagnostics& diagnostics,
        const std::string& source,
        const ResourceId& objectId
    )
    {
        if (
            object.initialState.empty() &&
            object.stateTransitions.empty()
            )
        {
            return;
        }

        if (object.initialState.empty())
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectInvalidStateMachine,
                "Compiled state machine is missing initial state",
                source,
                objectId + ".states.initial"
            );
        }

        if (object.stateTransitions.empty())
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectInvalidStateMachine,
                "Compiled state machine declares no states",
                source,
                objectId + ".states"
            );

            return;
        }

        if (!object.stateTransitions.contains(object.initialState))
        {
            diagnostics.error(
                DiagnosticCode::CompiledProjectInvalidStateMachine,
                "Compiled state machine initial state is not declared",
                source,
                objectId + ".states.initial"
            );
        }

        for (const auto& transition : object.stateTransitions)
        {
            if (transition.first.empty())
            {
                diagnostics.error(
                    DiagnosticCode::CompiledProjectInvalidStateMachine,
                    "Compiled state machine contains an empty state name",
                    source,
                    objectId + ".states"
                );
            }

            for (const std::string& target : transition.second)
            {
                if (
                    target.empty() ||
                    !object.stateTransitions.contains(target)
                    )
                {
                    diagnostics.error(
                        DiagnosticCode::CompiledProjectInvalidStateMachine,
                        "Compiled state machine transition points to a missing state",
                        source,
                        objectId + ".states." + transition.first + ".next"
                    );
                }
            }
        }
    }

    void validateCollider(
        const ObjectDefinition& object,
        const std::string& colliderName,
        const ColliderDefinition& collider,
        Diagnostics& diagnostics,
        const std::string& source,
        const ResourceId& objectId
    )
    {
        const std::string field =
            objectId + ".collisions." + colliderName;

        if (colliderName.empty())
        {
            diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Compiled collider name is empty",
                source,
                objectId + ".collisions"
            );
        }

        if (collider.type != "box" && collider.type != "ellipse")
        {
            diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Compiled collider type must be box or ellipse",
                source,
                field + ".type"
            );
        }

        const float effectiveWidth =
            collider.size.hasWidth
            ? collider.size.width
            : object.size.x;

        const float effectiveHeight =
            collider.size.hasHeight
            ? collider.size.height
            : object.size.y;

        if (effectiveWidth <= 0.0f || effectiveHeight <= 0.0f)
        {
            diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Compiled collider effective size must be positive",
                source,
                field + ".size"
            );
        }

        std::unordered_set<std::string> groups;

        for (const std::string& group : collider.with)
        {
            if (group.empty())
            {
                diagnostics.error(
                    DiagnosticCode::CompErrorUnclassified,
                    "Compiled collider with group cannot be empty",
                    source,
                    field + ".with"
                );
            }

            if (!groups.insert(group).second)
            {
                diagnostics.error(
                    DiagnosticCode::CompErrorUnclassified,
                    "Compiled collider with group is duplicated",
                    source,
                    field + ".with"
                );
            }
        }

        std::unordered_set<std::string> states;

        for (const std::string& state : collider.states)
        {
            if (state.empty())
            {
                diagnostics.error(
                    DiagnosticCode::CompErrorUnclassified,
                    "Compiled collider state cannot be empty",
                    source,
                    field + ".states"
                );
            }

            if (!states.insert(state).second)
            {
                diagnostics.error(
                    DiagnosticCode::CompErrorUnclassified,
                    "Compiled collider state is duplicated",
                    source,
                    field + ".states"
                );
            }
        }
    }
}

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

        if (objectId == project.rootId && object.component)
        {
            diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Compiled root object cannot be a component",
                source,
                objectId + ".component"
            );
        }

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

        if (
            object.controlPlayer < 0 ||
            object.controlPlayer > project.context.machine.input.players
            )
        {
            diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Compiled object control.player is outside Input Chip players",
                source,
                objectId + ".control.player"
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

        validateStateMachine(
            object,
            diagnostics,
            source,
            objectId
        );

        for (const auto& colliderPair : object.collisions)
        {
            validateCollider(
                object,
                colliderPair.first,
                colliderPair.second,
                diagnostics,
                source,
                objectId
            );
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

    validateAutomaticInstantiationCycles(
        project,
        diagnostics,
        source
    );

    return !diagnostics.hasErrors();
}
