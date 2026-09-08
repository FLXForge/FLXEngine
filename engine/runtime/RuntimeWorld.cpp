#include "RuntimeWorld.h"
#include "RuntimeHelpers.h"
#include "RuntimeObjectBuilder.h"
#include "../collision/CollisionGeometry.h"
#include "../compiler/CompiledProjectValidator.h"
#include "../collision/EffectiveColliderBuilder.h"
#include "../collision/CollisionSystem.h"
#include "../debug/Logger.h"
#include "../scripting/ScriptEngine.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <raylib.h>
#include <vector>

namespace
{
    constexpr size_t MaxLoadSpawnFlushPasses = 128;
    constexpr size_t MaxLoadSpawnedObjects = 4096;
    constexpr size_t MaxAutomaticInstantiationObjects = 4096;

    Vector2 rayDirection(float angle)
    {
        const float radians =
            (angle - 90.0f) * DEG2RAD;

        return Vector2{
            std::cos(radians),
            std::sin(radians)
        };
    }

    std::string logicalEntityRoot(
        const RuntimeObject& object,
        const std::vector<RuntimeObject>& objects
    )
    {
        const RuntimeObject* current =
            &object;

        while (current != nullptr && current->component && !current->parentId.empty())
        {
            const auto it =
                std::find_if(
                    objects.begin(),
                    objects.end(),
                    [current](const RuntimeObject& candidate)
                    {
                        return candidate.runtimeId == current->parentId;
                    }
                );

            if (it == objects.end())
            {
                break;
            }

            current =
                &*it;
        }

        return current == nullptr
            ? object.runtimeId
            : current->runtimeId;
    }

    bool sameLogicalEntity(
        const RuntimeObject& left,
        const RuntimeObject& right,
        const std::vector<RuntimeObject>& objects
    )
    {
        return logicalEntityRoot(left, objects) ==
            logicalEntityRoot(right, objects);
    }

    bool isComponentDescendantOf(
        const RuntimeObject& candidate,
        const std::string& ancestorId,
        const std::vector<RuntimeObject>& objects
    )
    {
        if (!candidate.component || candidate.parentId.empty())
        {
            return false;
        }

        if (candidate.parentId == ancestorId)
        {
            return true;
        }

        const auto parentIt =
            std::find_if(
                objects.begin(),
                objects.end(),
                [&candidate](const RuntimeObject& object)
                {
                    return object.runtimeId == candidate.parentId;
                }
            );

        if (parentIt == objects.end() || !parentIt->component)
        {
            return false;
        }

        return isComponentDescendantOf(
            *parentIt,
            ancestorId,
            objects
        );
    }

    Vector2 rotatedPoint(
        Vector2 center,
        Vector2 local,
        float angle
    )
    {
        const float radians =
            angle * DEG2RAD;

        const float cosine =
            std::cos(radians);

        const float sine =
            std::sin(radians);

        return Vector2{
            center.x + local.x * cosine - local.y * sine,
            center.y + local.x * sine + local.y * cosine
        };
    }

    Vector2 scaledPoint(Vector2 point, int scale)
    {
        return Vector2{
            point.x * scale,
            point.y * scale
        };
    }

    Color colliderDebugColor(const EffectiveCollider& collider)
    {
        if (!collider.declaredEnabled)
        {
            return Color{ 180, 70, 70, 255 };
        }

        if (!collider.stateAvailable)
        {
            return Color{ 230, 170, 40, 255 };
        }

        if (!collider.effective)
        {
            return Color{ 120, 120, 120, 255 };
        }

        return GREEN;
    }

    void drawDebugLine(Vector2 start, Vector2 end, int scale, Color color)
    {
        const Vector2 scaledStart =
            scaledPoint(start, scale);

        const Vector2 scaledEnd =
            scaledPoint(end, scale);

        DrawLine(
            static_cast<int>(std::round(scaledStart.x)),
            static_cast<int>(std::round(scaledStart.y)),
            static_cast<int>(std::round(scaledEnd.x)),
            static_cast<int>(std::round(scaledEnd.y)),
            color
        );
    }

    void drawDebugBox(
        const EffectiveCollider& collider,
        int scale,
        Color color
    )
    {
        const Vector2 corners[4] = {
            rotatedPoint(collider.center, Vector2{ -collider.halfSize.x, -collider.halfSize.y }, collider.angle),
            rotatedPoint(collider.center, Vector2{ collider.halfSize.x, -collider.halfSize.y }, collider.angle),
            rotatedPoint(collider.center, Vector2{ collider.halfSize.x, collider.halfSize.y }, collider.angle),
            rotatedPoint(collider.center, Vector2{ -collider.halfSize.x, collider.halfSize.y }, collider.angle)
        };

        for (int i = 0; i < 4; ++i)
        {
            drawDebugLine(
                corners[i],
                corners[(i + 1) % 4],
                scale,
                color
            );
        }
    }

    void drawDebugEllipse(
        const EffectiveCollider& collider,
        int scale,
        Color color
    )
    {
        constexpr int Segments = 40;

        Vector2 previous =
            rotatedPoint(
                collider.center,
                Vector2{ collider.halfSize.x, 0.0f },
                collider.angle
            );

        for (int i = 1; i <= Segments; ++i)
        {
            const float radians =
                static_cast<float>(i) / static_cast<float>(Segments) *
                2.0f * PI;

            const Vector2 current =
                rotatedPoint(
                    collider.center,
                    Vector2{
                        std::cos(radians) * collider.halfSize.x,
                        std::sin(radians) * collider.halfSize.y
                    },
                    collider.angle
                );

            drawDebugLine(previous, current, scale, color);
            previous =
                current;
        }
    }

    void drawDebugCollider(
        const EffectiveCollider& collider,
        int scale
    )
    {
        const Color color =
            colliderDebugColor(collider);

        if (collider.type == "ellipse")
        {
            drawDebugEllipse(collider, scale, color);
            return;
        }

        drawDebugBox(collider, scale, color);
    }

    void applyInheritedCreationMotion(
        RuntimeObject& child,
        const RuntimeObject& parent,
        const ObjectDefinition& definition
    )
    {
        if (definition.inherit.creationAngle == InheritCreationMode::Copy)
        {
            child.angle =
                parent.angle;
        }

        if (definition.inherit.creationVelocity == InheritCreationMode::Copy)
        {
            child.velocity =
                parent.velocity;
        }

        if (definition.inherit.creationVelocity == InheritCreationMode::Compose)
        {
            child.velocity.x +=
                parent.velocity.x;

            child.velocity.y +=
                parent.velocity.y;
        }
    }
}

RuntimeWorld::RuntimeWorld()
{
    nextRuntimeId = 1;
}

RuntimeLoadResult RuntimeWorld::load(
    const CompiledProject& project,
    ScriptEngine& scriptEngine
)
{
    RuntimeLoadResult result;

    objects.clear();
    pendingObjects.clear();
    collisionDebugFrame.clear();
    nextRuntimeId = 1;
    frameIndex = 0;
    resources = &project.resources;
    automaticInstantiationFailed = false;
    automaticInstantiationFailure.clear();

    scriptEngine.setFindObjectDefinitionFunction(
        [this](const std::string& id) -> const ObjectDefinition*
        {
            return resources == nullptr
                ? nullptr
                : resources->findObject(id);
        }
    );

    if (!CompiledProjectValidator::validate(
        project,
        result.diagnostics,
        "runtime"
    ))
    {
        for (const Diagnostic& diagnostic : result.diagnostics.all())
        {
            Logger::error(
                "runtime",
                diagnostic.message + ": " + diagnostic.field
            );
        }

        return result;
    }

    const ObjectDefinition* rootDefinition =
        resources == nullptr
        ? nullptr
        : resources->findObject(project.rootId);

    if (rootDefinition == nullptr)
    {
        result.diagnostics.error(
            DiagnosticCode::RuntimeWorldLoadFailed,
            "Compiled root resource not found",
            "runtime",
            project.rootId
        );

        Logger::error(
            "runtime",
            "Compiled root resource not found: " + project.rootId
        );

        return result;
    }

    RuntimeObject root =
        createRuntimeObject(
            *rootDefinition,
            project.rootId,
            ""
        );

    objects.push_back(
        std::move(root)
    );

    RuntimeObject rootSnapshot =
        objects.front();

    instantiateAutoChildren(
        rootSnapshot,
        objects
    );

    if (automaticInstantiationFailed)
    {
        result.diagnostics.error(
            DiagnosticCode::RuntimeWorldLoadFailed,
            automaticInstantiationFailure,
            "runtime",
            project.rootId
        );

        return result;
    }

    for (auto& object : objects)
    {
        loadScriptsForObject(object, scriptEngine);
        bornObject(object, scriptEngine);
    }

    if (!flushSpawnQueueForLoad(scriptEngine, result.diagnostics))
    {
        return result;
    }

    result.success =
        !result.diagnostics.hasErrors();

    return result;
}

void RuntimeWorld::update(
    ScriptEngine& scriptEngine,
    float screenWidth,
    float screenHeight,
    float delta
)
{
    ++frameIndex;
    scriptEngine.setRuntimeFrame(frameIndex);

    beginFrame();

    actionPhase(scriptEngine);
    flushSpawnQueue(scriptEngine);

    motionPhase(scriptEngine, screenWidth, screenHeight);
    flushSpawnQueue(scriptEngine);

    applyAttachments();

    CollisionSystem::run(
        objects,
        scriptEngine,
        collisionDebugEnabled
            ? &collisionDebugFrame
            : nullptr
    );
    flushSpawnQueue(scriptEngine);

    updateObjectTime(delta);

    deadPhase(scriptEngine);
    cleanupDeadObjects();
}

void RuntimeWorld::setCollisionDebugEnabled(bool enabled)
{
    collisionDebugEnabled =
        enabled;

    if (!collisionDebugEnabled)
    {
        collisionDebugFrame.clear();
    }
}

void RuntimeWorld::draw(
    ScriptEngine& scriptEngine,
    int screenScale,
    float screenWidth,
    float screenHeight,
    bool debugCollisions
)
{
    std::vector<RuntimeObject*> drawObjects;

    drawObjects.reserve(objects.size());

    for (auto& object : objects)
    {
        if (!object.alive || !object.visible)
        {
            continue;
        }

        drawObjects.push_back(&object);
    }

    std::stable_sort(
        drawObjects.begin(),
        drawObjects.end(),
        [](const RuntimeObject* left, const RuntimeObject* right)
        {
            return left->layer < right->layer;
        }
    );

    for (RuntimeObject* object : drawObjects)
    {
        if (!object->alive || !object->visible)
        {
            continue;
        }

        object->draw(
            screenScale,
            screenWidth,
            screenHeight
        );

        for (const auto& scriptPath : object->resolvedScriptPaths)
        {
            scriptEngine.callScriptFunction(
                scriptPath,
                "draw",
                *object
            );
        }
    }

    if (debugCollisions)
    {
        drawCollisionDebug(screenScale);
    }
}

void RuntimeWorld::spawn(
    RuntimeObject& source,
    const std::string& resourceId,
    ScriptEngine& scriptEngine
)
{
    if (resources == nullptr)
    {
        Logger::error(
            "spawn",
            "Resource registry is not available"
        );

        return;
    }

    const ObjectDefinition* definition =
        resources->findObject(resourceId);

    if (definition == nullptr)
    {
        Logger::error(
            "spawn",
            "Compiled child resource not found: " + resourceId
        );

        return;
    }

    const size_t firstQueuedIndex =
        pendingObjects.size();

    if (source.creationMode == "grid")
    {
        instantiateGridChildren(
            source,
            definition->id,
            "",
            pendingObjects
        );
    }
    else if (source.creationMode == "individual")
    {
        RuntimeObject instance =
            createIndividualChild(
                source,
                *definition,
                resourceId
            );

        pendingObjects.push_back(instance);

        RuntimeObject parentSnapshot =
            pendingObjects.back();

        instantiateAutoChildren(
            parentSnapshot,
            pendingObjects
        );
    }
    else
    {
        Logger::warning(
            "creation",
            "Unsupported creation mode '" + source.creationMode +
            "' in " + source.runtimeId
        );

        return;
    }

    for (
        size_t i = firstQueuedIndex;
        i < pendingObjects.size();
        ++i
        )
    {
        loadScriptsForObject(pendingObjects[i], scriptEngine);
    }

    if (pendingObjects.size() == firstQueuedIndex)
    {
        Logger::warning(
            "spawn",
            "No instances queued for child: " + definition->id
        );

        return;
    }

    Logger::debug(
        "spawn",
        "Queued child: " + definition->id
    );
}

void RuntimeWorld::kill(const std::string& runtimeId)
{
    RuntimeObject* object =
        findByRuntimeId(runtimeId);

    if (object == nullptr)
    {
        Logger::warning(
            "runtime",
            "kill target not found: " + runtimeId
        );

        return;
    }

    object->alive = false;

    for (RuntimeObject& candidate : objects)
    {
        if (!candidate.alive || candidate.runtimeId == runtimeId)
        {
            continue;
        }

        if (isComponentDescendantOf(candidate, runtimeId, objects))
        {
            candidate.alive = false;
        }
    }
}

void RuntimeWorld::show(const std::string& runtimeId)
{
    RuntimeObject* object =
        findByRuntimeId(runtimeId);

    if (object == nullptr)
    {
        Logger::warning(
            "runtime",
            "show target not found: " + runtimeId
        );

        return;
    }

    object->visible = true;
}

void RuntimeWorld::hide(const std::string& runtimeId)
{
    RuntimeObject* object =
        findByRuntimeId(runtimeId);

    if (object == nullptr)
    {
        Logger::warning(
            "runtime",
            "hide target not found: " + runtimeId
        );

        return;
    }

    object->visible = false;
}

RuntimeObject RuntimeWorld::createIndividualChild(
    const RuntimeObject& parent,
    const ObjectDefinition& definition,
    const std::string& resourceId
)
{
    RuntimeObject child =
        createRuntimeObject(
            definition,
            resourceId,
            parent.runtimeId
        );

    const float radians =
        parent.angle * DEG2RAD;

    const float rotatedX =
        definition.offset.x * std::cos(radians) -
        definition.offset.y * std::sin(radians);

    const float rotatedY =
        definition.offset.x * std::sin(radians) +
        definition.offset.y * std::cos(radians);

    if (definition.hasOffset)
    {
        child.position = Vector2{
            parent.position.x + rotatedX,
            parent.position.y + rotatedY
        };

        child.origin = child.position;
    }
    else if (!child.hasOrigin)
    {
        child.position = parent.position;
        child.origin = child.position;
    }

    applyInheritedCreationMotion(
        child,
        parent,
        definition
    );

    child.previousPosition =
        child.position;

    child.originalOffset = Vector2{
        child.position.x - parent.position.x,
        child.position.y - parent.position.y
    };

    return child;
}

RuntimeObject RuntimeWorld::createGridChild(
    const RuntimeObject& parent,
    const ObjectDefinition& definition,
    const std::string& resourceId,
    int row,
    int column
)
{
    RuntimeObject child =
        createRuntimeObject(
            definition,
            resourceId,
            parent.runtimeId
        );

    const float cellX =
        static_cast<float>(column) *
        parent.gridRules.cellWidth;

    const float cellY =
        static_cast<float>(row) *
        parent.gridRules.cellHeight;

    child.position = Vector2{
        parent.position.x + cellX + definition.offset.x,
        parent.position.y + cellY + definition.offset.y
    };

    child.origin =
        child.position;

    applyInheritedCreationMotion(
        child,
        parent,
        definition
    );

    child.previousPosition =
        child.position;

    child.originalOffset = Vector2{
        child.position.x - parent.position.x,
        child.position.y - parent.position.y
    };

    return child;
}

void RuntimeWorld::instantiateAutoChildren(
    const RuntimeObject& parent,
    std::vector<RuntimeObject>& target
)
{
    if (automaticInstantiationFailed)
    {
        return;
    }

    const bool rootCall =
        automaticInstantiationStack.empty();

    if (rootCall)
    {
        automaticInstantiationCreated = 0;
    }

    const std::string ancestryKey =
        parent.definitionId.empty()
            ? parent.sourcePath
            : parent.definitionId;

    if (
        !ancestryKey.empty()
        && std::find(
            automaticInstantiationStack.begin(),
            automaticInstantiationStack.end(),
            ancestryKey
        ) != automaticInstantiationStack.end()
        )
    {
        automaticInstantiationFailed = true;
        automaticInstantiationFailure =
            "Automatic instantiation cycle detected at object resource: " +
            ancestryKey;
        Logger::error(
            "creation",
            automaticInstantiationFailure
        );
        return;
    }

    automaticInstantiationStack.push_back(ancestryKey);

    if (parent.creationMode == "grid")
    {
        instantiateGridChildren(
            parent,
            "",
            "auto",
            target
        );

        automaticInstantiationStack.pop_back();
        return;
    }

    if (parent.creationMode != "individual")
    {
        Logger::warning(
            "creation",
            "Unsupported creation mode '" + parent.creationMode +
            "' in " + parent.runtimeId
        );

        automaticInstantiationStack.pop_back();
        return;
    }

    instantiateIndividualAutoChildren(
        parent,
        target
    );

    automaticInstantiationStack.pop_back();
}

RuntimeObject* RuntimeWorld::findByName(const std::string& name)
{
    for (auto& object : objects)
    {
        if (object.name == name)
        {
            return &object;
        }
    }

    return nullptr;
}

RuntimeObject* RuntimeWorld::findByRuntimeId(const std::string& id)
{
    for (auto& object : objects)
    {
        if (object.runtimeId == id)
        {
            return &object;
        }
    }

    return nullptr;
}

RuntimeObject* RuntimeWorld::findLiveByRuntimeId(const std::string& id)
{
    RuntimeObject* object =
        findByRuntimeId(id);

    return object != nullptr && object->alive
        ? object
        : nullptr;
}

std::vector<RuntimeObject*> RuntimeWorld::findAllLiveByName(
    const std::string& name
)
{
    std::vector<RuntimeObject*> matches;

    for (auto& object : objects)
    {
        if (object.alive && object.name == name)
        {
            matches.push_back(&object);
        }
    }

    return matches;
}

RuntimeObject* RuntimeWorld::findLiveParent(
    const std::string& runtimeId
)
{
    RuntimeObject* object =
        findByRuntimeId(runtimeId);

    if (object == nullptr || object->parentId.empty())
    {
        return nullptr;
    }

    return findLiveByRuntimeId(object->parentId);
}

std::vector<RuntimeObject*> RuntimeWorld::findLiveChildren(
    const std::string& runtimeId
)
{
    std::vector<RuntimeObject*> children;

    RuntimeObject* parent =
        findByRuntimeId(runtimeId);

    if (parent == nullptr)
    {
        return children;
    }

    for (auto& object : objects)
    {
        if (object.alive && object.parentId == runtimeId)
        {
            children.push_back(&object);
        }
    }

    return children;
}

void RuntimeWorld::keepOnly(const std::string& runtimeId)
{
    bool found =
        false;

    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        if (object.runtimeId == runtimeId)
        {
            found = true;
            continue;
        }

        kill(object.runtimeId);
    }

    if (!found)
    {
        Logger::warning(
            "runtime",
            "keep_only target not found: " + runtimeId
        );
    }
}

RayCastResult RuntimeWorld::rayCast(
    RuntimeObject& source,
    ScriptEngine& scriptEngine,
    float angle,
    float distance
)
{
    RayCastResult closest;

    if (!std::isfinite(angle) || !std::isfinite(distance))
    {
        Logger::warning(
            "ray",
            "ray requires finite angle and distance"
        );

        return closest;
    }

    if (distance < 0.0f)
    {
        Logger::warning(
            "ray",
            "ray distance cannot be negative"
        );

        return closest;
    }

    if (distance <= 0.0f)
    {
        return closest;
    }

    const Vector2 origin =
        source.position;

    const Vector2 direction =
        rayDirection(angle);

    CollisionDebugRay debugRay;
    debugRay.origin =
        origin;
    debugRay.end =
        Vector2{
            origin.x + direction.x * distance,
            origin.y + direction.y * distance
        };

    for (const RuntimeObject& target : objects)
    {
        if (!target.alive)
        {
            continue;
        }

        if (sameLogicalEntity(source, target, objects))
        {
            continue;
        }

        const std::vector<EffectiveCollider> colliders =
            EffectiveColliderBuilder::build(target, scriptEngine);

        for (const EffectiveCollider& collider : colliders)
        {
            const RayCollisionHit hit =
                CollisionGeometry::ray(
                    collider,
                    origin,
                    direction,
                    distance
                );

            if (!hit.hit)
            {
                continue;
            }

            if (!closest.hit || hit.distance < closest.distance)
            {
                closest.hit = true;
                closest.objectId = target.runtimeId;
                closest.group = target.group;
                closest.collider = collider.name;
                closest.distance = hit.distance;
                closest.point = hit.point;
                closest.normal = hit.normal;

                debugRay.hit = true;
                debugRay.hitPoint = hit.point;
                debugRay.hitNormal = hit.normal;
                debugRay.targetId = target.runtimeId;
                debugRay.collider = collider.name;
            }
        }
    }

    if (collisionDebugEnabled)
    {
        collisionDebugFrame.rays.push_back(debugRay);
    }

    return closest;
}

RuntimeObject RuntimeWorld::createRuntimeObject(
    const ObjectDefinition& definition,
    const std::string& resourceId,
    const std::string& parentId
)
{
    const std::string runtimeId =
        createRuntimeId(definition.id);

    RuntimeObject object =
        RuntimeObjectBuilder::build(
        definition,
        runtimeId,
        parentId
    );

    if (!object.state.empty())
    {
        object.stateEnteredFrame =
            frameIndex == 0 ? 0 : frameIndex + 1;
    }

    object.definitionId =
        resourceId.empty()
            ? definition.id
            : resourceId;

    return object;
}

void RuntimeWorld::instantiateIndividualAutoChildren(
    const RuntimeObject& parent,
    std::vector<RuntimeObject>& target
)
{
    if (resources == nullptr)
    {
        Logger::error(
            "creation",
            "Resource registry is not available"
        );

        return;
    }

    for (const auto& pair : parent.childResources)
    {
        const ObjectDefinition* definition =
            resources->findObject(pair.second);

        if (definition == nullptr)
        {
            Logger::error(
                "creation",
                "Compiled child resource not found: " + pair.second
            );

            continue;
        }

        if (definition->spawnMode != "auto")
        {
            continue;
        }

        RuntimeObject child =
            createIndividualChild(
                parent,
                *definition,
                pair.second
            );

        ++automaticInstantiationCreated;

        if (automaticInstantiationCreated > MaxAutomaticInstantiationObjects)
        {
            automaticInstantiationFailed = true;
            automaticInstantiationFailure =
                "Automatic instantiation limit exceeded under object: " +
                parent.name;
            Logger::error(
                "creation",
                automaticInstantiationFailure
            );
            return;
        }

        std::vector<RuntimeObject> descendants;

        instantiateAutoChildren(
            child,
            descendants
        );

        target.push_back(
            std::move(child)
        );

        target.insert(
            target.end(),
            std::make_move_iterator(descendants.begin()),
            std::make_move_iterator(descendants.end())
        );
    }
}

void RuntimeWorld::instantiateGridChildren(
    const RuntimeObject& parent,
    const std::string& requestedChildId,
    const std::string& requestedSpawnMode,
    std::vector<RuntimeObject>& target
)
{
    if (resources == nullptr)
    {
        Logger::error(
            "creation",
            "Resource registry is not available"
        );

        return;
    }

    if (!validGridCreation(parent))
    {
        return;
    }

    for (int row = 0; row < parent.gridRules.rows; ++row)
    {
        for (int column = 0; column < parent.gridRules.columns; ++column)
        {
            std::string childId;

            if (!gridChildIdAt(
                parent,
                row,
                column,
                childId
            ))
            {
                continue;
            }

            if (!requestedChildId.empty() && childId != requestedChildId)
            {
                continue;
            }

            const auto it =
                parent.childResources.find(childId);

            if (it == parent.childResources.end())
            {
                Logger::warning(
                    "creation",
                    "Grid pattern references missing child '" +
                    childId + "' in " + parent.runtimeId
                );

                continue;
            }

            const ObjectDefinition* definition =
                resources->findObject(it->second);

            if (definition == nullptr)
            {
                Logger::error(
                    "creation",
                    "Compiled grid child resource not found: " +
                    it->second
                );

                continue;
            }

            if (
                !requestedSpawnMode.empty() &&
                definition->spawnMode != requestedSpawnMode
                )
            {
                continue;
            }

            RuntimeObject child =
                createGridChild(
                    parent,
                    *definition,
                    it->second,
                    row,
                    column
                );

            ++automaticInstantiationCreated;

            if (automaticInstantiationCreated > MaxAutomaticInstantiationObjects)
            {
                automaticInstantiationFailed = true;
                automaticInstantiationFailure =
                    "Automatic grid instantiation limit exceeded under object: " +
                    parent.name;
                Logger::error(
                    "creation",
                    automaticInstantiationFailure
                );
                return;
            }

            std::vector<RuntimeObject> descendants;

            instantiateAutoChildren(
                child,
                descendants
            );

            target.push_back(
                std::move(child)
            );

            target.insert(
                target.end(),
                std::make_move_iterator(descendants.begin()),
                std::make_move_iterator(descendants.end())
            );
        }
    }
}

bool RuntimeWorld::gridChildIdAt(
    const RuntimeObject& parent,
    int row,
    int column,
    std::string& childId
) const
{
    if (parent.gridPatternIsRows)
    {
        if (parent.gridRowPattern.empty())
        {
            return false;
        }

        const std::vector<std::string>& rowPattern =
            parent.gridRowPattern[
                row % static_cast<int>(parent.gridRowPattern.size())
            ];

        if (rowPattern.empty())
        {
            return false;
        }

        childId =
            rowPattern[
                column % static_cast<int>(rowPattern.size())
            ];

        return true;
    }

    if (parent.gridPattern.empty())
    {
        return false;
    }

    const int index =
        row * parent.gridRules.columns + column;

    childId =
        parent.gridPattern[
            index % static_cast<int>(parent.gridPattern.size())
        ];

    return true;
}

bool RuntimeWorld::validGridCreation(
    const RuntimeObject& parent
) const
{
    if (
        parent.gridRules.rows <= 0 ||
        parent.gridRules.columns <= 0 ||
        parent.gridRules.cellWidth <= 0.0f ||
        parent.gridRules.cellHeight <= 0.0f
        )
    {
        Logger::error(
            "creation",
            "Invalid grid creation rules in " + parent.runtimeId
        );

        return false;
    }

    if (
        (!parent.gridPatternIsRows && parent.gridPattern.empty()) ||
        (parent.gridPatternIsRows && parent.gridRowPattern.empty())
        )
    {
        Logger::error(
            "creation",
            "Grid creation pattern is empty in " + parent.runtimeId
        );

        return false;
    }

    return true;
}

void RuntimeWorld::loadScriptsForObject(
    RuntimeObject& object,
    ScriptEngine& scriptEngine
)
{
    for (const auto& scriptPath : object.resolvedScriptPaths)
    {
        if (resources == nullptr)
        {
            Logger::error(
                "script",
                "Resource registry is not available"
            );

            continue;
        }

        const ScriptResource* script =
            resources->findScript(scriptPath);

        if (script == nullptr)
        {
            Logger::error(
                "script",
                "Compiled script resource not found: " + scriptPath
            );

            continue;
        }

        scriptEngine.loadScript(
            script->id,
            script->code
        );
    }
}

void RuntimeWorld::bornObject(
    RuntimeObject& object,
    ScriptEngine& scriptEngine
)
{
    for (const auto& scriptPath : object.resolvedScriptPaths)
    {
        scriptEngine.callScriptFunction(
            scriptPath,
            "born",
            object
        );
    }
}

void RuntimeWorld::flushSpawnQueue(ScriptEngine& scriptEngine)
{
    std::vector<RuntimeObject> queuedObjects =
        std::move(pendingObjects);

    pendingObjects.clear();

    for (auto& object : queuedObjects)
    {
        objects.push_back(
            std::move(object)
        );

        bornObject(
            objects.back(),
            scriptEngine
        );

        Logger::debug(
            "spawn",
            "Spawned instance"
        );
    }
}

bool RuntimeWorld::flushSpawnQueueForLoad(
    ScriptEngine& scriptEngine,
    Diagnostics& diagnostics
)
{
    size_t passCount = 0;
    size_t spawnedCount = 0;

    while (!pendingObjects.empty())
    {
        if (passCount >= MaxLoadSpawnFlushPasses)
        {
            diagnostics.error(
                DiagnosticCode::RuntimeLoadSpawnLimitExceeded,
                "Runtime load did not stabilize while flushing spawn requests from born",
                "runtime",
                "born.spawn"
            );

            Logger::error(
                "runtime",
                "Runtime load spawn flush pass limit exceeded"
            );

            return false;
        }

        spawnedCount +=
            pendingObjects.size();

        if (spawnedCount > MaxLoadSpawnedObjects)
        {
            diagnostics.error(
                DiagnosticCode::RuntimeLoadSpawnLimitExceeded,
                "Runtime load created too many objects while flushing spawn requests from born",
                "runtime",
                "born.spawn"
            );

            Logger::error(
                "runtime",
                "Runtime load spawn object limit exceeded"
            );

            return false;
        }

        ++passCount;
        flushSpawnQueue(scriptEngine);

        if (automaticInstantiationFailed)
        {
            diagnostics.error(
                DiagnosticCode::RuntimeWorldLoadFailed,
                automaticInstantiationFailure,
                "runtime",
                "born.spawn"
            );

            return false;
        }
    }

    return true;
}

void RuntimeWorld::beginFrame()
{
    collisionDebugFrame.clear();

    for (auto& object : objects)
    {
        object.previousPosition =
            object.position;
        RuntimeHelpers::beginMechanicsFrame(object);

        if (
            !object.state.empty() &&
            object.stateEnteredFrame == 0
            )
        {
            object.stateEnteredFrame =
                frameIndex;
        }
    }
}

void RuntimeWorld::actionPhase(ScriptEngine& scriptEngine)
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        for (const auto& scriptPath : object.resolvedScriptPaths)
        {
            scriptEngine.callScriptFunction(
                scriptPath,
                "action",
                object
            );
        }
    }
}

void RuntimeWorld::motionPhase(
    ScriptEngine& scriptEngine,
    float screenWidth,
    float screenHeight
)
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        for (const auto& scriptPath : object.resolvedScriptPaths)
        {
            scriptEngine.callScriptFunction(
                scriptPath,
                "motion",
                object
            );
        }

        RuntimeHelpers::applyFreeMechanics(
            object,
            scriptEngine.getFrameDelta()
        );

        object.applyBounds(screenWidth, screenHeight);
    }
}

void RuntimeWorld::drawPhase(ScriptEngine& scriptEngine)
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        for (const auto& scriptPath : object.resolvedScriptPaths)
        {
            scriptEngine.callScriptFunction(
                scriptPath,
                "draw",
                object
            );
        }
    }
}

void RuntimeWorld::applyAttachments()
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        if (object.parentId.empty())
        {
            continue;
        }

        RuntimeObject* parent =
            findByRuntimeId(object.parentId);

        if (parent == nullptr || !parent->alive)
        {
            continue;
        }

        if (object.attached && object.attachFollowX)
        {
            object.position.x =
                parent->position.x + object.originalOffset.x;
        }

        if (object.attached && object.attachFollowY)
        {
            object.position.y =
                parent->position.y + object.originalOffset.y;
        }

        if (object.attached && object.attachFollowAngle)
        {
            object.angle =
                parent->angle;
        }

        if (object.inherit.liveAngle == InheritLiveMode::Copy)
        {
            object.angle =
                parent->angle;
        }
    }
}

void RuntimeWorld::deadPhase(ScriptEngine& scriptEngine)
{
    for (auto& object : objects)
    {
        if (object.alive || object.deadCalled)
        {
            continue;
        }

        for (const auto& scriptPath : object.resolvedScriptPaths)
        {
            scriptEngine.callScriptFunction(
                scriptPath,
                "dead",
                object
            );
        }

        object.deadCalled = true;
    }
}

void RuntimeWorld::cleanupDeadObjects()
{
    objects.erase(
        std::remove_if(
            objects.begin(),
            objects.end(),
            [](const RuntimeObject& object)
            {
                return !object.alive;
            }
        ),
        objects.end()
    );
}

void RuntimeWorld::updateObjectTime(float delta)
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        if (!object.state.empty())
        {
            if (object.stateEnteredFrame <= frameIndex)
            {
                object.stateTime +=
                    delta;
            }
        }

        for (auto& timer : object.timers)
        {
            if (timer.second.status != RuntimeTimerStatus::Running)
            {
                continue;
            }

            timer.second.left =
                std::max(
                    0.0f,
                    timer.second.left - delta
                );

            if (timer.second.left <= 0.0f)
            {
                timer.second.left = 0.0f;
                timer.second.status = RuntimeTimerStatus::Done;
            }
        }
    }
}

void RuntimeWorld::drawCollisionDebug(int screenScale) const
{
    for (const EffectiveCollider& collider : collisionDebugFrame.colliders)
    {
        drawDebugCollider(collider, screenScale);
    }

    for (const CollisionDebugContact& contact : collisionDebugFrame.contacts)
    {
        const Vector2 point =
            contact.contact.point;

        DrawCircle(
            static_cast<int>(std::round(point.x * screenScale)),
            static_cast<int>(std::round(point.y * screenScale)),
            std::max(2.0f, 2.0f * static_cast<float>(screenScale)),
            YELLOW
        );

        drawDebugLine(
            point,
            Vector2{
                point.x + contact.contact.normal.x * 10.0f,
                point.y + contact.contact.normal.y * 10.0f
            },
            screenScale,
            YELLOW
        );
    }

    for (const CollisionDebugRay& ray : collisionDebugFrame.rays)
    {
        drawDebugLine(
            ray.origin,
            ray.end,
            screenScale,
            Color{ 80, 180, 255, 255 }
        );

        if (!ray.hit)
        {
            continue;
        }

        DrawCircle(
            static_cast<int>(std::round(ray.hitPoint.x * screenScale)),
            static_cast<int>(std::round(ray.hitPoint.y * screenScale)),
            std::max(2.0f, 2.0f * static_cast<float>(screenScale)),
            SKYBLUE
        );

        drawDebugLine(
            ray.hitPoint,
            Vector2{
                ray.hitPoint.x + ray.hitNormal.x * 10.0f,
                ray.hitPoint.y + ray.hitNormal.y * 10.0f
            },
            screenScale,
            BLUE
        );
    }
}

std::string RuntimeWorld::createRuntimeId(const std::string& name)
{
    return name + "_" + std::to_string(nextRuntimeId++);
}
