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

    RuntimeObject root =
        createRuntimeObject(rootDefinition, "");

    objects.push_back(
        std::move(root)
    );

    instantiateAutoChildren(
        objects.front(),
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
    float screenHeight
)
{
    actionPhase(scriptEngine);
    flushSpawnQueue(scriptEngine);

    motionPhase(scriptEngine, screenWidth, screenHeight);
    flushSpawnQueue(scriptEngine);

    CollisionSystem::run(objects, scriptEngine);
    flushSpawnQueue(scriptEngine);

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
    for (const auto& object : objects)
    {
        object.draw(
            screenScale,
            screenWidth,
            screenHeight
        );

        if (debugCollisions)
        {
            object.drawCollision(screenScale);
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
    RuntimeObject instance =
        createRuntimeObject(
            definition,
            source.runtimeId
        );

    const float radians =
        source.angle * DEG2RAD;

    const float rotatedX =
        definition.offset.x * std::cos(radians) -
        definition.offset.y * std::sin(radians);

    const float rotatedY =
        definition.offset.x * std::sin(radians) +
        definition.offset.y * std::cos(radians);

    if (definition.hasOffset)
    {
        instance.position = Vector2{
            source.position.x + rotatedX,
            source.position.y + rotatedY
        };

        instance.origin = instance.position;
    }
    else if (instance.hasOrigin)
    {
        instance.position = instance.origin;
    }
    else
    {
        instance.position = source.position;
        instance.origin = instance.position;
    }

    instance.angle =
        source.angle;

    loadScriptsForObject(instance, scriptEngine);

    const size_t firstQueuedIndex =
        pendingObjects.size();

    pendingObjects.push_back(instance);

    instantiateAutoChildren(
        pendingObjects.back(),
        pendingObjects
    );

    for (
        size_t i = firstQueuedIndex + 1;
        i < pendingObjects.size();
        ++i
        )
    {
        loadScriptsForObject(pendingObjects[i], scriptEngine);
    }

    Logger::debug(
        "spawn",
        "Queued instance: " + instance.name
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

RuntimeObject RuntimeWorld::createRuntimeObject(
    const ObjectDefinition& definition,
    const std::string& parentId
)
{
    return RuntimeObjectBuilder::build(
        definition,
        createRuntimeId(definition.id),
        parentId
    );
}

void RuntimeWorld::instantiateAutoChildren(
    const RuntimeObject& parent,
    std::vector<RuntimeObject>& target
)
{
    const auto children =
        parent.children;

    const std::string parentRuntimeId =
        parent.runtimeId;

    const Vector2 parentPosition =
        parent.position;

    const float parentAngle =
        parent.angle;

    for (const auto& pair : children)
    {
        const ObjectDefinition& definition =
            pair.second;

        if (definition.spawnMode != "auto")
        {
            continue;
        }

        RuntimeObject child =
            createRuntimeObject(
                definition,
                parentRuntimeId
            );

        if (definition.hasOffset)
        {
            const float radians =
                parentAngle * DEG2RAD;

            const float rotatedX =
                definition.offset.x * std::cos(radians) -
                definition.offset.y * std::sin(radians);

            const float rotatedY =
                definition.offset.x * std::sin(radians) +
                definition.offset.y * std::cos(radians);

            child.position = Vector2{
                parentPosition.x + rotatedX,
                parentPosition.y + rotatedY
            };

            child.origin = child.position;
        }
        else if (!child.hasOrigin)
        {
            child.position = parentPosition;
            child.origin = child.position;
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

std::string RuntimeWorld::createRuntimeId(const std::string& name)
{
    return name + "_" + std::to_string(nextRuntimeId++);
}
