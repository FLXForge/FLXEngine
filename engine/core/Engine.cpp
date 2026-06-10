#include "Engine.h"
#include "../runtime/RuntimeHelpers.h"
#include "../runtime/RuntimeConstants.h"
#include "../runtime/RuntimeObjectBuilder.h"
#include "../loading/JsonLoader.h"
#include "../project/FlxContextBuilder.h"
#include "../scripting/ScriptEngine.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <raylib.h>
#include <filesystem>

namespace
{
    bool canCollideWith(
        const RuntimeObject& object,
        const RuntimeObject& other
    )
    {
        if (!object.collisionActive)
        {
            return false;
        }

        if (object.collisionWith.empty())
        {
            return false;
        }

        return std::find(
            object.collisionWith.begin(),
            object.collisionWith.end(),
            other.group
        ) != object.collisionWith.end();
    }
}

Engine::Engine()
{
    nextRuntimeId = 1;
}

void Engine::run(const std::string& flxPath)
{
    SetTraceLogCallback(Logger::rayLibLog);

    init(flxPath);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    shutdown();
}

void Engine::init(const std::string& flxPath)
{
    loadProject(flxPath);
    initWindow();
    configureScriptEngine();
    loadScripts();

    SetTargetFPS(60);
}

void Engine::loadProject(const std::string& flxPath)
{
    context =
        FlxContextBuilder::build(flxPath);

    Logger::setDebugEnabled(context.debugLogs);

    Logger::info(
        "project",
        "Loaded project: " + context.name
    );

    std::string rootPath =
        JsonLoader::resolveProjectPath(
            context.projectPath,
            context.root,
            ".json"
        );

    Logger::info(
        "project",
        "Loaded root: " + rootPath
    );

    objects.clear();

    rootDefinition =
        JsonLoader::loadObjectDefinition(rootPath);

    RuntimeObject root =
        createRuntimeObject(rootDefinition, "");

    objects.push_back(
        std::move(root)
    );

    instantiateAutoChildren(
        objects.front(),
        objects
    );

    scriptEngine.setScreenScale(context.screenScale);
}

void Engine::initWindow()
{
    const std::string title =
        context.screenTitle.empty()
        ? "Flx"
        : context.screenTitle;

    InitWindow(
        context.screenWidth * context.screenScale,
        context.screenHeight * context.screenScale,
        title.c_str()
    );
}

void Engine::bornObject(RuntimeObject& object)
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

void Engine::flushSpawnQueue()
{
    for (auto& object : pendingObjects)
    {
        bornObject(object);

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

void Engine::configureScriptEngine()
{
    scriptEngine.setFindObjectFunction(
        [this](const std::string& name)
        {
            return find(name);
        }
    );

    scriptEngine.setFindObjectByIdFunction(
        [this](const std::string& id)
        {
            return findByRuntimeId(id);
        }
    );

    scriptEngine.setSpawnObjectFunction(
        [this](
            RuntimeObject& source,
            const ObjectDefinition& definition
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

            loadScriptsForObject(instance);

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
                loadScriptsForObject(pendingObjects[i]);
            }

            Logger::debug(
                "spawn",
                "Queued instance: " + instance.name
            );
        }
    );
}

RuntimeObject* Engine::findByRuntimeId(
    const std::string& id
)
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

void Engine::loadScripts()
{
    for (auto& object : objects)
    {
        loadScriptsForObject(object);

        bornObject(object);
    }
}

void Engine::loadScriptsForObject(RuntimeObject& object)
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

RuntimeObject Engine::createRuntimeObject(
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

void Engine::instantiateAutoChildren(
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

std::string Engine::createRuntimeId(
    const std::string& name
)
{
    return name + "_" + std::to_string(nextRuntimeId++);
}

void Engine::update()
{
    actionPhase();
    flushSpawnQueue();

    motionPhase();
    flushSpawnQueue();

    collisionPhase();
    flushSpawnQueue();

    deadPhase();
    cleanupDeadObjects();
}

void Engine::actionPhase()
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        if (!object.scripts.empty())
        {
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
}

void Engine::motionPhase()
{
    for (auto& object : objects)
    {
        if (!object.alive)
        {
            continue;
        }

        if (!object.scripts.empty())
        {
            for (const auto& scriptPath : object.resolvedScriptPaths)
            {
                scriptEngine.callScriptFunction(
                    scriptPath,
                    "motion",
                    object
                );
            }
        }
        object.applyBounds(
            static_cast<float>(context.screenWidth),
            static_cast<float>(context.screenHeight)
        );
    }
}

void Engine::collisionPhase()
{
    for (size_t i = 0; i < objects.size(); ++i)
    {
        RuntimeObject& a =
            objects[i];

        if (!a.alive || !a.collisionActive)
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

            if (!canCollideWith(a, b))
            {
                continue;
            }

            if (RuntimeHelpers::intersects(a, b))
            {
                for (const auto& scriptPath : a.resolvedScriptPaths)
                {
                    scriptEngine.callScriptFunction(
                        scriptPath,
                        "collision",
                        a,
                        b
                    );
                }
            }
        }
    }
}

void Engine::drawPhase()
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

void Engine::deadPhase()
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

void Engine::cleanupDeadObjects()
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

void Engine::draw()
{
    BeginDrawing();

    ClearBackground(BLACK);

    for (const auto& object : objects)
    {
        object.draw(
            context.screenScale,
            static_cast<float>(context.screenWidth),
            static_cast<float>(context.screenHeight)
        );

        if (context.debugCollisions)
        {
            object.drawCollision(
                context.screenScale
            );
        }
    }

    drawPhase();

    EndDrawing();
}

void Engine::shutdown()
{
    CloseWindow();
}

RuntimeObject* Engine::find(const std::string& name)
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
