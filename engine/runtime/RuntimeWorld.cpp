#include "RuntimeWorld.h"
#include "RuntimeObjectBuilder.h"
#include "../compiler/CompiledProjectValidator.h"
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
    Vector2 rayDirection(float angle)
    {
        const float radians =
            (angle - 90.0f) * DEG2RAD;

        return Vector2{
            std::cos(radians),
            std::sin(radians)
        };
    }

    Vector2 collisionCenter(const RuntimeObject& object)
    {
        if (object.shapeType == "block")
        {
            return Vector2{
                object.position.x + object.size.x / 2.0f,
                object.position.y + object.size.y / 2.0f
            };
        }

        return object.position;
    }

    float collisionRadius(const RuntimeObject& object)
    {
        if (object.collisionRadius > 0.0f)
        {
            return object.collisionRadius;
        }

        return std::max(
            object.size.x,
            object.size.y
        ) / 2.0f;
    }

    bool rayHitsCircle(
        Vector2 origin,
        Vector2 direction,
        float maxDistance,
        const RuntimeObject& object,
        RayCastResult& result
    )
    {
        const Vector2 center =
            collisionCenter(object);

        const float radius =
            collisionRadius(object);

        const float ox =
            origin.x - center.x;

        const float oy =
            origin.y - center.y;

        const float b =
            2.0f * (ox * direction.x + oy * direction.y);

        const float c =
            ox * ox + oy * oy - radius * radius;

        const float discriminant =
            b * b - 4.0f * c;

        if (discriminant < 0.0f)
        {
            return false;
        }

        const float root =
            std::sqrt(discriminant);

        float distance =
            (-b - root) / 2.0f;

        if (distance < 0.0f)
        {
            distance =
                (-b + root) / 2.0f;
        }

        if (distance < 0.0f || distance > maxDistance)
        {
            return false;
        }

        result.hit = true;
        result.group = object.group;
        result.distance = distance;
        result.point = Vector2{
            origin.x + direction.x * distance,
            origin.y + direction.y * distance
        };

        return true;
    }

    bool rayHitsBox(
        Vector2 origin,
        Vector2 direction,
        float maxDistance,
        const RuntimeObject& object,
        RayCastResult& result
    )
    {
        const Rectangle box = Rectangle{
            object.position.x,
            object.position.y,
            object.size.x,
            object.size.y
        };

        float tMin = 0.0f;
        float tMax = maxDistance;

        const auto updateAxis = [](
            float originValue,
            float directionValue,
            float minValue,
            float maxValue,
            float& tMin,
            float& tMax
        )
        {
            if (std::abs(directionValue) < 0.00001f)
            {
                return originValue >= minValue && originValue <= maxValue;
            }

            float nearDistance =
                (minValue - originValue) / directionValue;

            float farDistance =
                (maxValue - originValue) / directionValue;

            if (nearDistance > farDistance)
            {
                std::swap(
                    nearDistance,
                    farDistance
                );
            }

            tMin =
                std::max(tMin, nearDistance);

            tMax =
                std::min(tMax, farDistance);

            return tMin <= tMax;
        };

        if (!updateAxis(
            origin.x,
            direction.x,
            box.x,
            box.x + box.width,
            tMin,
            tMax
        ))
        {
            return false;
        }

        if (!updateAxis(
            origin.y,
            direction.y,
            box.y,
            box.y + box.height,
            tMin,
            tMax
        ))
        {
            return false;
        }

        if (tMin < 0.0f || tMin > maxDistance)
        {
            return false;
        }

        result.hit = true;
        result.group = object.group;
        result.distance = tMin;
        result.point = Vector2{
            origin.x + direction.x * tMin,
            origin.y + direction.y * tMin
        };

        return true;
    }

    bool groupMatches(
        const RuntimeObject& source,
        const RuntimeObject& target
    )
    {
        return std::find(
            source.collisionWith.begin(),
            source.collisionWith.end(),
            target.group
        ) != source.collisionWith.end();
    }

    bool collisionTypeSupported(const RuntimeObject& object)
    {
        return object.collisionType == "circle" ||
            object.collisionType == "box";
    }

    void applyInheritedCreationMotion(
        RuntimeObject& child,
        const RuntimeObject& parent,
        const ObjectDefinition& definition
    )
    {
        if (!definition.inheritParentAngle)
        {
            return;
        }

        if (!definition.hasAngle)
        {
            child.angle =
                parent.angle;
        }

        if (!definition.hasSpeed && definition.maxSpeed > 0.0f)
        {
            child.speed =
                definition.maxSpeed;
            child.originSpeed =
                definition.maxSpeed;
        }

        child.velocity.x +=
            parent.velocity.x;

        child.velocity.y +=
            parent.velocity.y;
    }
}

RuntimeWorld::RuntimeWorld()
{
    nextRuntimeId = 1;
}

void RuntimeWorld::load(
    const CompiledProject& project,
    ScriptEngine& scriptEngine
)
{
    objects.clear();
    pendingObjects.clear();
    nextRuntimeId = 1;
    frameIndex = 0;
    resources = &project.resources;

    Diagnostics validationDiagnostics;

    if (!CompiledProjectValidator::validate(
        project,
        validationDiagnostics,
        DiagnosticCode::RuntimeErrorUnclassified,
        "runtime"
    ))
    {
        for (const Diagnostic& diagnostic : validationDiagnostics.all())
        {
            Logger::error(
                "runtime",
                diagnostic.message + ": " + diagnostic.field
            );
        }

        return;
    }

    const ObjectDefinition* rootDefinition =
        resources == nullptr
        ? nullptr
        : resources->findObject(project.rootId);

    if (rootDefinition == nullptr)
    {
        Logger::error(
            "runtime",
            "Compiled root resource not found: " + project.rootId
        );

        return;
    }

    RuntimeObject root =
        createRuntimeObject(*rootDefinition, "");

    objects.push_back(
        std::move(root)
    );

    RuntimeObject rootSnapshot =
        objects.front();

    instantiateAutoChildren(
        rootSnapshot,
        objects
    );

    for (auto& object : objects)
    {
        loadScriptsForObject(object, scriptEngine);
        bornObject(object, scriptEngine);
    }
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

    CollisionSystem::run(objects, scriptEngine);
    flushSpawnQueue(scriptEngine);

    updateObjectTime(delta);

    deadPhase(scriptEngine);
    cleanupDeadObjects();
}

void RuntimeWorld::draw(
    ScriptEngine& scriptEngine,
    int screenScale,
    float screenWidth,
    float screenHeight,
    bool debugCollisions
)
{
    std::vector<const RuntimeObject*> drawObjects;

    drawObjects.reserve(objects.size());

    for (const auto& object : objects)
    {
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

    for (const RuntimeObject* object : drawObjects)
    {
        object->draw(
            screenScale,
            screenWidth,
            screenHeight
        );

        if (debugCollisions)
        {
            object->drawCollision(screenScale);
        }
    }

    drawPhase(scriptEngine);
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
                *definition
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

RuntimeObject RuntimeWorld::createIndividualChild(
    const RuntimeObject& parent,
    const ObjectDefinition& definition
)
{
    RuntimeObject child =
        createRuntimeObject(
            definition,
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
    int row,
    int column
)
{
    RuntimeObject child =
        createRuntimeObject(
            definition,
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
    if (parent.creationMode == "grid")
    {
        instantiateGridChildren(
            parent,
            "",
            "auto",
            target
        );

        return;
    }

    if (parent.creationMode != "individual")
    {
        Logger::warning(
            "creation",
            "Unsupported creation mode '" + parent.creationMode +
            "' in " + parent.runtimeId
        );

        return;
    }

    instantiateIndividualAutoChildren(
        parent,
        target
    );
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

        object.alive = false;
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
    const RuntimeObject& source,
    float angle,
    float distance
) const
{
    RayCastResult closest;

    if (distance <= 0.0f)
    {
        return closest;
    }

    if (source.collisionWith.empty())
    {
        Logger::warning(
            "ray",
            "ray source has no collision.with groups"
        );

        return closest;
    }

    const Vector2 origin =
        source.position;

    const Vector2 direction =
        rayDirection(angle);

    for (const RuntimeObject& target : objects)
    {
        if (!target.alive)
        {
            continue;
        }

        if (target.runtimeId == source.runtimeId)
        {
            continue;
        }

        if (!groupMatches(source, target))
        {
            continue;
        }

        if (!collisionTypeSupported(target))
        {
            continue;
        }

        RayCastResult candidate;

        if (target.collisionType == "circle")
        {
            rayHitsCircle(
                origin,
                direction,
                distance,
                target,
                candidate
            );
        }
        else if (target.collisionType == "box")
        {
            rayHitsBox(
                origin,
                direction,
                distance,
                target,
                candidate
            );
        }

        if (!candidate.hit)
        {
            continue;
        }

        if (!closest.hit || candidate.distance < closest.distance)
        {
            closest =
                candidate;
        }
    }

    return closest;
}

RuntimeObject RuntimeWorld::createRuntimeObject(
    const ObjectDefinition& definition,
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
            frameIndex;
    }

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
                *definition
            );

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
                    row,
                    column
                );

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

void RuntimeWorld::beginFrame()
{
    for (auto& object : objects)
    {
        object.previousPosition =
            object.position;

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
        if (!object.alive || !object.attached)
        {
            continue;
        }

        if (object.originalParentId.empty())
        {
            continue;
        }

        RuntimeObject* parent =
            findByRuntimeId(object.originalParentId);

        if (parent == nullptr || !parent->alive)
        {
            continue;
        }

        if (object.attachFollowX)
        {
            object.position.x =
                parent->position.x + object.originalOffset.x;
        }

        if (object.attachFollowY)
        {
            object.position.y =
                parent->position.y + object.originalOffset.y;
        }

        if (object.attachFollowAngle)
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
            object.stateTime +=
                delta;
        }

        for (auto& timer : object.timers)
        {
            timer.second.left =
                std::max(
                    0.0f,
                    timer.second.left - delta
                );
        }
    }
}

std::string RuntimeWorld::createRuntimeId(const std::string& name)
{
    return name + "_" + std::to_string(nextRuntimeId++);
}
