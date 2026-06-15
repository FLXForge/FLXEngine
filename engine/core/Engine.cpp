#include "Engine.h"
#include "../debug/Logger.h"
#include "../loading/JsonLoader.h"
#include "../project/FlxContextBuilder.h"

#include <raylib.h>
#include <algorithm>

Engine::Engine() = default;

namespace
{
    constexpr float MaxFrameDelta =
        1.0f / 30.0f;
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

    const std::string rootPath =
        JsonLoader::resolveProjectPath(
            context.projectPath,
            context.root,
            ".json"
        );

    Logger::info(
        "project",
        "Loaded root: " + rootPath
    );

    initWindow();
    audioSystem.init();
    scriptEngine.setScreenScale(context.screenScale);
    configureScriptEngine();

    const ObjectDefinition rootDefinition =
        JsonLoader::loadObjectDefinition(rootPath);

    world.load(rootDefinition, scriptEngine);
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

void Engine::configureScriptEngine()
{
    scriptEngine.setFadeSystem(&fadeSystem);
    scriptEngine.setAudioSystem(&audioSystem);

    scriptEngine.setFindObjectFunction(
        [this](const std::string& name)
        {
            return world.findByName(name);
        }
    );

    scriptEngine.setFindObjectByIdFunction(
        [this](const std::string& id)
        {
            return world.findByRuntimeId(id);
        }
    );

    scriptEngine.setSpawnObjectFunction(
        [this](
            RuntimeObject& source,
            const ObjectDefinition& definition
            )
        {
            world.spawn(
                source,
                definition,
                scriptEngine
            );
        }
    );

    scriptEngine.setRayCastFunction(
        [this](
            RuntimeObject& source,
            float angle,
            float distance
            )
        {
            return world.rayCast(
                source,
                angle,
                distance
            );
        }
    );

    scriptEngine.setKeepOnlyFunction(
        [this](const std::string& runtimeId)
        {
            world.keepOnly(runtimeId);
        }
    );
}

void Engine::update()
{
    const float delta =
        safeFrameDelta();

    scriptEngine.setFrameDelta(delta);

    world.update(
        scriptEngine,
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight)
    );

    fadeSystem.update(delta);
    audioSystem.update();
}

void Engine::draw()
{
    BeginDrawing();

    ClearBackground(BLACK);

    world.draw(
        scriptEngine,
        context.screenScale,
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight),
        context.debugCollisions
    );

    fadeSystem.draw(
        context.screenWidth,
        context.screenHeight,
        context.screenScale
    );

    EndDrawing();
}

void Engine::shutdown()
{
    audioSystem.shutdown();
    CloseWindow();
}

RuntimeObject* Engine::find(const std::string& name)
{
    return world.findByName(name);
}

float Engine::safeFrameDelta() const
{
    const float rawDelta =
        GetFrameTime();

    const float delta =
        std::min(rawDelta, MaxFrameDelta);

    if (rawDelta > MaxFrameDelta)
    {
        Logger::debug(
            "time",
            "Frame delta clamped from " +
            std::to_string(rawDelta) +
            " to " +
            std::to_string(delta)
        );
    }

    return delta;
}
