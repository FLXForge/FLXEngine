#include "Engine.h"
#include "../runtime/RuntimeHelpers.h"
#include "../runtime/RuntimeConstants.h"
#include "../loading/JsonLoader.h"
#include "../project/FlxManifestLoader.h"
#include "../scripting/ScriptEngine.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <iostream>
#include <raylib.h>
#include <filesystem>

Engine::Engine()
{
    screenWidth = 320;
    screenHeight = 180;
    screenScale = 3;
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
    FlxManifest manifest =
        FlxManifestLoader::load(flxPath);

    Logger::info(
        "project",
        "Loaded project: " + manifest.name
    );

    projectBasePath = manifest.rootDirectory;

    if (!manifest.path.empty() && manifest.path != "./")
    {
        projectBasePath += "/" + manifest.path;
    }

    std::string mainPath =
        resolveJsonPath(projectBasePath, manifest.main);

    gameConfig =
        JsonLoader::loadGameConfig(mainPath);

    Logger::info(
        "project",
        "Loaded main: " + gameConfig.name
    );

    objects.clear();

    for (const auto& child : gameConfig.children)
    {
        const std::string childPath =
            resolveJsonPath(projectBasePath, child);

        Logger::info(
            "project",
            "Loaded child: " + childPath
        );

        std::vector<RuntimeObject> childObjects =
            JsonLoader::loadObjects(childPath);

        objects.insert(
            objects.end(),
            childObjects.begin(),
            childObjects.end()
        );
    }

    loadPrefabs();

    screenWidth = gameConfig.screenWidth;
    screenHeight = gameConfig.screenHeight;
    screenScale = gameConfig.scale;
}

void Engine::initWindow()
{
    const std::string title =
        gameConfig.screenTitle.empty()
        ? "Flx"
        : gameConfig.screenTitle;

    InitWindow(
        screenWidth * screenScale,
        screenHeight * screenScale,
        title.c_str()
    );
}

void Engine::bornObject(RuntimeObject& object)
{
    Logger::debug("Llamadas a born");
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

    scriptEngine.setSpawnObjectFunction(
        [this](
            RuntimeObject& source,
            const SpawnDefinition& spawnDefinition,
            const RuntimeObject& prefab
            )
        {
            RuntimeObject instance = prefab;

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

void Engine::loadScripts()
{
    for (const auto& programScript : gameConfig.programScripts) {
        const std::string programScriptPath =
            resolveScriptPath(projectBasePath, programScript);

        scriptEngine.loadScript(programScriptPath);

        scriptEngine.callScriptFunction(programScriptPath, "start");
    }

    for (auto& object : objects)
    {
        for (const auto& script : object.scripts)
        {
            const std::string scriptPath =
                resolveScriptPath(projectBasePath, script);

            object.resolvedScriptPaths.push_back(scriptPath);

            scriptEngine.loadScript(scriptPath);
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
            const std::string& spawnName =
                pair.first;

            const SpawnDefinition& spawnDefinition =
                pair.second;

            if (prefabs.contains(spawnDefinition.prefab))
            {
                continue;
            }

            const std::string prefabPath =
                resolveJsonPath(
                    projectBasePath,
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

                continue;
            }

            prefabs.emplace(
                spawnDefinition.prefab,
                prefabObjects.front()
            );

            Logger::info(
                "project",
                "Loaded prefab: " + spawnDefinition.prefab
            );
        }
    }
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
            static_cast<float>(gameConfig.screenWidth),
            static_cast<float>(gameConfig.screenHeight)
        );
    }
}

void Engine::collisionPhase()
{
    for (size_t i = 0; i < objects.size(); ++i)
    {
        for (size_t j = i + 1; j < objects.size(); ++j)
        {
            RuntimeObject& a = objects[i];
            RuntimeObject& b = objects[j];

            if (!a.alive || !b.alive)
            {
                continue;
            }

            if (RuntimeHelpers::intersects(a, b))
            {
                if (!a.resolvedScriptPaths.empty())
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

                if (!b.resolvedScriptPaths.empty())
                {
                    for (const auto& scriptPath : b.resolvedScriptPaths)
                    {
                        scriptEngine.callScriptFunction(
                            scriptPath,
                            "collision",
                            b,
                            a
                        );
                    }
                }
            }
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
            gameConfig.scale,
            static_cast<float>(gameConfig.screenWidth),
            static_cast<float>(gameConfig.screenHeight)
        );
    }

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
