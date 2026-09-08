#include "CollisionSystem.h"
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
        const RuntimeObject& object
    )
    {
        return std::any_of(
            object.collisions.begin(),
            object.collisions.end(),
            [](const auto& pair)
            {
                return pair.second.enabled && !pair.second.with.empty();
            }
        );
    }
}

void CollisionSystem::run(
    std::vector<RuntimeObject>& objects,
    ScriptEngine& scriptEngine
)
{
    for (size_t i = 0; i < objects.size(); ++i)
    {
        RuntimeObject& a =
            objects[i];

        if (!a.alive || !hasAnyDirectedCollider(a))
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

            const std::vector<EffectiveCollider> sourceColliders =
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
}
