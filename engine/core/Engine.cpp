#include "Engine.h"
#include "../runtime/RuntimeHelpers.h"
#include "../runtime/RuntimeConstants.h"
#include "../loading/JsonLoader.h"
#include "../project/FlxManifestLoader.h"
#include "../scripting/ScriptEngine.h"
#include "../debug/Logger.h"

#include <iostream>
#include <raylib.h>

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
        "Loaded game: " + gameConfig.name
    );

    objects.clear();

    for (const auto& child : gameConfig.children)
    {
        const std::string childPath =
            resolveJsonPath(projectBasePath, child);

        std::vector<RuntimeObject> childObjects =
            JsonLoader::loadObjects(childPath);

        objects.insert(
            objects.end(),
            childObjects.begin(),
            childObjects.end()
        );
    }

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

void Engine::configureScriptEngine()
{
    scriptEngine.setFindObjectFunction(
        [this](const std::string& name)
        {
            return find(name);
        }
    );
}

void Engine::loadScripts()
{
    for (const auto& programScript : gameConfig.programScripts) {
        const std::string programScriptPath =
            resolveScriptPath(projectBasePath, programScript);

        scriptEngine.loadScript(programScriptPath);

        scriptEngine.callScriptFunction(programScriptPath, "gameStart");
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

    return basePath + "/" + resolved;
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

    return basePath + "/" + resolved;
}

void Engine::update()
{
    actionPhase();
    motionPhase();
    collisionPhase();
}

void Engine::actionPhase()
{
    for (auto& object : objects)
    {
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

void Engine::draw()
{
    BeginDrawing();

    ClearBackground(BLACK);

    for (const auto& object : objects)
    {
        object.draw(screenScale);
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
