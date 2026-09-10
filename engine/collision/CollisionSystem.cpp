#include "CollisionSystem.h"
#include "CollisionDebugFrame.h"
#include "CollisionGeometry.h"
#include "EffectiveColliderBuilder.h"
#include "../runtime/RuntimeObject.h"
#include "../scripting/ScriptEngine.h"

#include <algorithm>

namespace
{
#ifdef FLX_TESTING
    using StatsPointer = CollisionSystemStats*;

    void countSourceBuild(StatsPointer stats)
    {
        if (stats != nullptr)
        {
            ++stats->sourceBuilds;
        }
    }

    void countTargetBuild(StatsPointer stats)
    {
        if (stats != nullptr)
        {
            ++stats->targetBuilds;
        }
    }

    void countCandidateObject(StatsPointer stats)
    {
        if (stats != nullptr)
        {
            ++stats->candidateObjects;
        }
    }

    void countNarrowPhaseCall(StatsPointer stats)
    {
        if (stats != nullptr)
        {
            ++stats->narrowPhaseCalls;
        }
    }

    void countCallbackInvocation(StatsPointer stats)
    {
        if (stats != nullptr)
        {
            ++stats->callbackInvocations;
        }
    }
#else
    using StatsPointer = void*;

    void countSourceBuild(StatsPointer) {}
    void countTargetBuild(StatsPointer) {}
    void countCandidateObject(StatsPointer) {}
    void countNarrowPhaseCall(StatsPointer) {}
    void countCallbackInvocation(StatsPointer) {}
#endif

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

    bool hasDirectedGroup(
        const std::vector<EffectiveCollider>& sourceColliders,
        const RuntimeObject& target
    )
    {
        return std::any_of(
            sourceColliders.begin(),
            sourceColliders.end(),
            [&target](const EffectiveCollider& collider)
            {
                return
                    collider.effective &&
                    hasDirectedGroup(collider, target);
            }
        );
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

    class LinearCollisionCandidateProvider
    {
    public:
        LinearCollisionCandidateProvider(
            std::vector<RuntimeObject>& objects,
            size_t sourceIndex
        )
            : objects(objects),
              sourceIndex(sourceIndex),
              objectCount(objects.size())
        {
        }

        RuntimeObject* next(
            const std::vector<EffectiveCollider>& sourceColliders,
            StatsPointer stats
        )
        {
            while (cursor < objectCount)
            {
                const size_t targetIndex =
                    cursor;

                ++cursor;

                if (targetIndex == sourceIndex)
                {
                    continue;
                }

                RuntimeObject& target =
                    objects[targetIndex];

                if (!target.alive)
                {
                    continue;
                }

                if (!hasDirectedGroup(sourceColliders, target))
                {
                    continue;
                }

                countCandidateObject(stats);

                return &target;
            }

            return nullptr;
        }

    private:
        std::vector<RuntimeObject>& objects;
        size_t sourceIndex = 0;
        size_t objectCount = 0;
        size_t cursor = 0;
    };

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

    void runCollisionSystem(
        std::vector<RuntimeObject>& objects,
        ScriptEngine& scriptEngine,
        CollisionDebugFrame* debugFrame,
        StatsPointer stats
    )
    {
        const size_t objectCount =
            objects.size();

        for (size_t i = 0; i < objectCount; ++i)
        {
            RuntimeObject& a =
                objects[i];

            if (!a.alive)
            {
                continue;
            }

            std::vector<EffectiveCollider> sourceColliders;

            bool sourceDirty =
                true;

            const auto rebuildSource =
                [&]()
                {
                    countSourceBuild(stats);

                    sourceColliders =
                        EffectiveColliderBuilder::build(a, scriptEngine);

                    sourceDirty =
                        false;

                    return hasAnyDirectedCollider(sourceColliders);
                };

            if (!rebuildSource())
            {
                continue;
            }

            LinearCollisionCandidateProvider candidates(objects, i);

            while (a.alive)
            {
                if (sourceDirty && !rebuildSource())
                {
                    break;
                }

                RuntimeObject* target =
                    candidates.next(sourceColliders, stats);

                if (target == nullptr)
                {
                    break;
                }

                RuntimeObject& b =
                    *target;

                countTargetBuild(stats);

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

                        countNarrowPhaseCall(stats);

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

                bool callbackExecuted =
                    false;

                for (const auto& scriptPath : a.resolvedScriptPaths)
                {
                    countCallbackInvocation(stats);

                    callbackExecuted =
                        true;

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

                sourceDirty =
                    callbackExecuted;
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
}

void CollisionSystem::run(
    std::vector<RuntimeObject>& objects,
    ScriptEngine& scriptEngine,
    CollisionDebugFrame* debugFrame
)
{
    runCollisionSystem(
        objects,
        scriptEngine,
        debugFrame,
        nullptr
    );
}

#ifdef FLX_TESTING
void CollisionSystem::run(
    std::vector<RuntimeObject>& objects,
    ScriptEngine& scriptEngine,
    CollisionSystemStats& stats,
    CollisionDebugFrame* debugFrame
)
{
    runCollisionSystem(
        objects,
        scriptEngine,
        debugFrame,
        &stats
    );
}
#endif
