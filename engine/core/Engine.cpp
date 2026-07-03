#include "Engine.h"
#include "../debug/Logger.h"
#include "../loading/JsonLoader.h"
#include "../machine/VideoColorProcessor.h"
#include "../project/FlxContextBuilder.h"
#include "../tools/ColorParser.h"

#include <raylib.h>
#include <algorithm>

Engine::Engine() = default;

namespace
{
    constexpr float MaxFrameDelta =
        1.0f / 30.0f;

    void projectDefinitionColors(
        ObjectDefinition& definition,
        const VideoChipDefinition& video
    )
    {
        definition.color =
            VideoColorProcessor::project(
                definition.color,
                video
            );

        for (auto& child : definition.children)
        {
            projectDefinitionColors(
                child.second,
                video
            );
        }
    }
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
    initVideoOutput();
    audioSystem.configure(context.machine.audio);
    audioSystem.init();
    scriptEngine.setScreenScale(1);
    configureScriptEngine();

    ObjectDefinition rootDefinition =
        JsonLoader::loadObjectDefinition(rootPath);

    projectDefinitionColors(
        rootDefinition,
        context.machine.video
    );

    world.load(rootDefinition, scriptEngine);
}

void Engine::initWindow()
{
    const std::string title =
        context.screenTitle.empty()
        ? "Flx"
        : context.screenTitle;

    Logger::info(
        "graphics",
        "Window size: " +
        std::to_string(context.screenWidth * context.screenScale) +
        "x" +
        std::to_string(context.screenHeight * context.screenScale)
    );

    InitWindow(
        context.screenWidth * context.screenScale,
        context.screenHeight * context.screenScale,
        title.c_str()
    );
}

void Engine::initVideoOutput()
{
    backgroundColor =
        ColorParser::parse(
            context.machine.video.clearColor,
            BLACK
        );

    backgroundColor =
        VideoColorProcessor::project(
            backgroundColor,
            context.machine.video
        );

    Logger::info(
        "graphics",
        "Logical screen: " +
        std::to_string(context.screenWidth) +
        "x" +
        std::to_string(context.screenHeight) +
        " scale " +
        std::to_string(context.screenScale)
    );

    renderTarget =
        LoadRenderTexture(
            context.screenWidth,
            context.screenHeight
        );

    renderTargetLoaded = true;

    SetTextureFilter(
        renderTarget.texture,
        context.machine.video.smoothing
        ? TEXTURE_FILTER_BILINEAR
        : TEXTURE_FILTER_POINT
    );
}

void Engine::configureScriptEngine()
{
    scriptEngine.setVideoChip(&context.machine.video);
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
        static_cast<float>(context.screenHeight),
        delta
    );

    fadeSystem.update(delta);
    audioSystem.update();
}

void Engine::draw()
{
    BeginTextureMode(renderTarget);

    ClearBackground(backgroundColor);

    world.draw(
        scriptEngine,
        1,
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight),
        context.debugCollisions
    );

    fadeSystem.draw(
        context.screenWidth,
        context.screenHeight,
        1
    );

    EndTextureMode();

    BeginDrawing();

    ClearBackground(backgroundColor);

    DrawTexturePro(
        renderTarget.texture,
        Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(renderTarget.texture.width),
            static_cast<float>(-renderTarget.texture.height)
        },
        Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(context.screenWidth * context.screenScale),
            static_cast<float>(context.screenHeight * context.screenScale)
        },
        Vector2{ 0.0f, 0.0f },
        0.0f,
        WHITE
    );

    EndDrawing();
}

void Engine::shutdown()
{
    shutdownVideoOutput();
    audioSystem.shutdown();
    CloseWindow();
}

void Engine::shutdownVideoOutput()
{
    if (!renderTargetLoaded)
    {
        return;
    }

    UnloadRenderTexture(renderTarget);
    renderTargetLoaded = false;
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
