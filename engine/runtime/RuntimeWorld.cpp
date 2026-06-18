#include "RuntimeWorld.h"
#include "RuntimeObjectBuilder.h"
#include "../collision/CollisionSystem.h"
#include "../debug/Logger.h"
#include "../loading/JsonLoader.h"
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
}

RuntimeWorld::RuntimeWorld()
{
    nextRuntimeId = 1;
}

void RuntimeWorld::load(
    const ObjectDefinition& rootDefinition,
    ScriptEngine& scriptEngine
)
{
    objects.clear();
    pendingObjects.clear();
    nextRuntimeId = 1;
    frameIndex = 0;

    RuntimeObject root =
        createRuntimeObject(rootDefinition, "");

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
    const ObjectDefinition& definition,
    ScriptEngine& scriptEngine
)
{
    const size_t firstQueuedIndex =
        pendingObjects.size();

    if (source.creationMode == "grid")
    {
        instantiateGridChildren(
            source,
            definition.id,
            "",
            pendingObjects
        );
    }
    else if (source.creationMode == "individual")
    {
        RuntimeObject instance =
            createIndividualChild(
                source,
                definition,
                true
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
            "No instances queued for child: " + definition.id
        );

        return;
    }

    Logger::debug(
        "spawn",
        "Queued child: " + definition.id
    );
}

RuntimeObject RuntimeWorld::createIndividualChild(
    const RuntimeObject& parent,
    const ObjectDefinition& definition,
    bool inheritParentAngle
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

    child.previousPosition =
        child.position;

    child.originalOffset = Vector2{
        child.position.x - parent.position.x,
        child.position.y - parent.position.y
    };

    if (inheritParentAngle)
    {
        child.angle =
            parent.angle;
    }

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
    RuntimeObject object =
        RuntimeObjectBuilder::build(
        definition,
        createRuntimeId(definition.id),
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
    const auto children =
        parent.children;

    for (const auto& pair : children)
    {
        const ObjectDefinition& definition =
            pair.second;

        if (definition.spawnMode != "auto")
        {
            continue;
        }

        RuntimeObject child =
            createIndividualChild(
                parent,
                definition,
                false
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
                parent.children.find(childId);

            if (it == parent.children.end())
            {
                Logger::warning(
                    "creation",
                    "Grid pattern references missing child '" +
                    childId + "' in " + parent.runtimeId
                );

                continue;
            }

            const ObjectDefinition& definition =
                it->second;

            if (
                !requestedSpawnMode.empty() &&
                definition.spawnMode != requestedSpawnMode
                )
            {
                continue;
            }

            RuntimeObject child =
                createGridChild(
                    parent,
                    definition,
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
    object.resolvedScriptPaths.clear();

    for (const auto& script : object.scripts)
    {
        const std::string scriptPath =
            JsonLoader::resolveReferencedPath(
                object.sourcePath,
                script,
                ".js"
            );

        object.resolvedScriptPaths.push_back(scriptPath);

        scriptEngine.loadScript(scriptPath);
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
    for (auto& object : pendingObjects)
    {
        bornObject(object, scriptEngine);

        objects.push_back(
            std::move(object)
        );

        Logger::debug(
            "spawn",
            "Spawned instance"
        );
    }

    pendingObjects.clear();
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

        for (auto it = object.timers.begin(); it != object.timers.end();)
        {
            it->second.left =
                std::max(
                    0.0f,
                    it->second.left - delta
                );

            if (it->second.left <= 0.0f)
            {
                it =
                    object.timers.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
}

std::string RuntimeWorld::createRuntimeId(const std::string& name)
{
    return name + "_" + std::to_string(nextRuntimeId++);
}
