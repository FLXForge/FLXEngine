#include "Engine.h"
#include "../runtime/RuntimeHelpers.h"
#include "../runtime/RuntimeConstants.h"
#include "../loading/JsonLoader.h"
#include "../project/FlxContextBuilder.h"
#include "../scripting/ScriptEngine.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <iostream>
#include <raylib.h>
#include <filesystem>
#include <unordered_set>

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

    Logger::info(
        "project",
        "Loaded project: " + context.name
    );

    projectBasePath = context.projectPath;

    std::string rootPath =
        resolveJsonPath(projectBasePath, context.root);

    Logger::info(
        "project",
        "Loaded root: " + rootPath
    );

    objects.clear();

    objects =
        JsonLoader::loadObjects(rootPath);

    for (auto& object : objects)
    {
        object.runtimeId =
            createRuntimeId(object.name);
    }

    loadPrefabs();

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

        Logger::info(
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

    scriptEngine.setFindPrefabFunction(
        [this](const std::string& name) -> RuntimeObject*
        {
            auto it = prefabs.find(name);

            if (it == prefabs.end())
            {
                return nullptr;
            }

            return &it->second;
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
            const SpawnDefinition& spawnDefinition,
            const RuntimeObject& prefab
            )
        {
            RuntimeObject instance = prefab;

            instance.runtimeId =
                createRuntimeId(instance.name);
            instance.alive = true;
            instance.deadCalled = false;
            instance.local.clear();
            instance.resolvedScriptPaths.clear();

            const float radians =
                source.angle * DEG2RAD;

            const float rotatedX =
                spawnDefinition.offset.x * std::cos(radians) -
                spawnDefinition.offset.y * std::sin(radians);

            const float rotatedY =
                spawnDefinition.offset.x * std::sin(radians) +
                spawnDefinition.offset.y * std::cos(radians);

            if (spawnDefinition.hasOffset)
            {
                const float radians =
                    source.angle * DEG2RAD;

                const float rotatedX =
                    spawnDefinition.offset.x * std::cos(radians) -
                    spawnDefinition.offset.y * std::sin(radians);

                const float rotatedY =
                    spawnDefinition.offset.x * std::sin(radians) +
                    spawnDefinition.offset.y * std::cos(radians);

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

            instance.angle = source.angle;

            instance.origin =
                instance.position;

            instance.angle =
                source.angle;

            for (const auto& script : instance.scripts)
            {
                const std::string scriptPath =
                    resolveScriptPath(projectBasePath, script);

                instance.resolvedScriptPaths.push_back(scriptPath);

                scriptEngine.loadScript(scriptPath);

            }

            pendingObjects.push_back(instance);

            Logger::info(
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
    std::unordered_set<std::string> startedScripts;

    for (auto& object : objects)
    {
        for (const auto& script : object.scripts)
        {
            const std::string scriptPath =
                resolveScriptPath(projectBasePath, script);

            object.resolvedScriptPaths.push_back(scriptPath);

            scriptEngine.loadScript(scriptPath);

            if (startedScripts.insert(scriptPath).second)
            {
                scriptEngine.callScriptFunction(scriptPath, "start");
            }
        }

        bornObject(object);
    }
}

void Engine::loadPrefabs()
{
    for (const auto& object : objects)
    {
        for (const auto& pair : object.spawns)
        {
            loadPrefabRecursive(pair.second);
        }
    }
}

void Engine::loadPrefabRecursive(
    const SpawnDefinition& spawnDefinition
)
{
    if (prefabs.contains(spawnDefinition.prefab))
    {
        return;
    }

    const std::string prefabPath =
        resolveJsonPath(
            spawnDefinition.basePath,
            spawnDefinition.prefab
        );

    std::vector<RuntimeObject> prefabObjects =
        JsonLoader::loadObjects(prefabPath);

    if (prefabObjects.empty())
    {
        Logger::warning(
            "project",
            "Prefab could not be loaded: " +
            spawnDefinition.prefab
        );

        return;
    }

    RuntimeObject prefab =
        prefabObjects.front();

    prefabs.emplace(
        spawnDefinition.prefab,
        prefab
    );

    Logger::info(
        "project",
        "Loaded prefab: " + spawnDefinition.prefab
    );

    for (const auto& pair : prefab.spawns)
    {
        loadPrefabRecursive(pair.second);
    }
}

std::string Engine::createRuntimeId(
    const std::string& name
)
{
    return name + "_" + std::to_string(nextRuntimeId++);
}

std::string Engine::resolveJsonPath(
    const std::string& basePath,
    const std::string& file
) const
{
    std::string resolved = file;

    if (resolved.find(".json") == std::string::npos)
    {
        resolved += ".json";
    }

    return std::filesystem::path(basePath + "/" + resolved).generic_string();
}

std::string Engine::resolveScriptPath(
    const std::string& basePath,
    const std::string& file
) const
{
    std::string resolved = file;

    if (resolved.find(".js") == std::string::npos)
    {
        resolved += ".js";
    }

    return std::filesystem::path(basePath + "/" + resolved).generic_string();
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
