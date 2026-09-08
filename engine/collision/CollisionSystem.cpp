#include "CollisionSystem.h"
#include "CollisionDebugFrame.h"
#include "CollisionGeometry.h"
#include "EffectiveColliderBuilder.h"
#include "../runtime/RuntimeObject.h"
#include "../scripting/ScriptEngine.h"

#include <algorithm>

namespace
{
    bool hasDirectedGroup(
        const EffectiveCollider& source,
        const RuntimeObject& target
    )
    {
        if (source.with.empty())
        {
            return false;
        }

        return std::find(
            source.with.begin(),
            source.with.end(),
            target.group
        ) != source.with.end();
    }

    bool hasAnyDirectedCollider(
        const std::vector<EffectiveCollider>& colliders
    )
    {
        return std::any_of(
            colliders.begin(),
            colliders.end(),
            [](const EffectiveCollider& collider)
            {
                return collider.effective && !collider.with.empty();
            }
        );
    }

    void captureFrameColliders(
        const std::vector<RuntimeObject>& objects,
        ScriptEngine& scriptEngine,
        CollisionDebugFrame& debugFrame
    )
    {
        debugFrame.colliders.clear();

        for (const RuntimeObject& object : objects)
        {
            if (!object.alive)
            {
                continue;
            }

            std::vector<EffectiveCollider> colliders =
                EffectiveColliderBuilder::build(object, scriptEngine);

            debugFrame.colliders.insert(
                debugFrame.colliders.end(),
                colliders.begin(),
                colliders.end()
            );
        }
    }
}

void CollisionSystem::run(
    std::vector<RuntimeObject>& objects,
    ScriptEngine& scriptEngine,
    CollisionDebugFrame* debugFrame
)
{
    for (size_t i = 0; i < objects.size(); ++i)
    {
        RuntimeObject& a =
            objects[i];

        if (!a.alive)
        {
            continue;
        }

        std::vector<EffectiveCollider> sourceColliders =
            EffectiveColliderBuilder::build(a, scriptEngine);

        if (!hasAnyDirectedCollider(sourceColliders))
        {
            continue;
        }

        for (size_t j = 0; j < objects.size(); ++j)
        {
            if (i == j)
            {
                continue;
            }

            RuntimeObject& b =
                objects[j];

            if (!b.alive)
            {
                continue;
            }

            sourceColliders =
                EffectiveColliderBuilder::build(a, scriptEngine);

            const std::vector<EffectiveCollider> targetColliders =
                EffectiveColliderBuilder::build(b, scriptEngine);

            if (sourceColliders.empty() || targetColliders.empty())
            {
                continue;
            }

            std::vector<CollisionContact> contacts;

            for (const EffectiveCollider& sourceCollider : sourceColliders)
            {
                if (!sourceCollider.effective ||
                    !hasDirectedGroup(sourceCollider, b))
                {
                    continue;
                }

                for (const EffectiveCollider& targetCollider : targetColliders)
                {
                    CollisionContact contact;

                    if (CollisionGeometry::contact(
                        sourceCollider,
                        targetCollider,
                        contact
                    ))
                    {
                        contacts.push_back(contact);
                    }
                }
            }

            if (contacts.empty())
            {
                continue;
            }

            if (debugFrame != nullptr)
            {
                for (const CollisionContact& contact : contacts)
                {
                    debugFrame->contacts.push_back(
                        CollisionDebugContact{
                            a.runtimeId,
                            b.runtimeId,
                            contact
                        }
                    );
                }
            }

            for (const auto& scriptPath : a.resolvedScriptPaths)
            {
                scriptEngine.callScriptFunction(
                    scriptPath,
                    "collision",
                    a,
                    b,
                    contacts
                );

                if (!a.alive || !b.alive)
                {
                    break;
                }
            }

            if (!a.alive)
            {
                break;
            }
        }
    }

    if (debugFrame != nullptr)
    {
        captureFrameColliders(
            objects,
            scriptEngine,
            *debugFrame
        );
    }
}
