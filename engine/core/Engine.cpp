#include "Engine.h"
#include "../debug/Logger.h"
#include "../machine/VideoColorProcessor.h"
#include "../tools/ColorParser.h"

#include <raylib.h>
#include <algorithm>
#include <cmath>

Engine::Engine()
    : world(std::make_unique<RuntimeWorld>())
{
}

namespace
{
    constexpr float MaxFrameDelta =
        1.0f / 30.0f;

    Rectangle renderDestination(
        const FlxContext& context
    )
    {
        if (context.windowMode != "fullscreen")
        {
            return Rectangle{
                0.0f,
                0.0f,
                static_cast<float>(context.screenWidth * context.screenScale),
                static_cast<float>(context.screenHeight * context.screenScale)
            };
        }

        const float windowWidth =
            static_cast<float>(GetScreenWidth());

        const float windowHeight =
            static_cast<float>(GetScreenHeight());

        const float horizontalScale =
            windowWidth / static_cast<float>(context.screenWidth);

        const float verticalScale =
            windowHeight / static_cast<float>(context.screenHeight);

        float scale =
            std::min(horizontalScale, verticalScale);

        if (scale >= 1.0f)
        {
            scale =
                std::floor(scale);
        }

        if (scale <= 0.0f)
        {
            scale = 1.0f;
        }

        const float width =
            static_cast<float>(context.screenWidth) * scale;

        const float height =
            static_cast<float>(context.screenHeight) * scale;

        return Rectangle{
            (windowWidth - width) * 0.5f,
            (windowHeight - height) * 0.5f,
            width,
            height
        };
    }
}

void Engine::run(const CompiledProject& project, int maxFrames)
{
    SetTraceLogCallback(Logger::rayLibLog);

    init(project);

    int frameCount = 0;

    while (!WindowShouldClose() && !scriptEngine.exitRequested())
    {
        update();

        if (scriptEngine.exitRequested())
        {
            break;
        }

        draw();

        ++frameCount;

        if (maxFrames >= 0 && frameCount >= maxFrames)
        {
            break;
        }
    }

    shutdown();
}

void Engine::init(const CompiledProject& project)
{
    loadProject(project);

    SetTargetFPS(60);
}

void Engine::loadProject(const CompiledProject& project)
{
    context =
        project.context;

    Logger::setConsoleEnabled(context.debugConsole);
    Logger::setDebugEnabled(context.debugLogs);
    SetTraceLogLevel(
        context.debugConsole
        ? LOG_ALL
        : LOG_NONE
    );

    Logger::info(
        "project",
        "Loaded project: " + context.name
    );

    Logger::info(
        "project",
        "Loaded root: " + project.rootPath
    );

    initWindow();
    initVideoOutput();
    audioSystem.configure(context.machine.audio);
    audioSystem.init();
    inputSystem.configure(context.machine.input);

    if (!context.inputMappingContent.empty())
    {
        inputSystem.loadMappingContent(
            context.inputMappingSourceName,
            context.inputMappingContent
        );
    }
    else
    {
        inputSystem.loadMapping(context.inputMappingPath);
    }

    scriptEngine.setScreenScale(1);
    configureScriptEngine();
    world->load(project, scriptEngine);
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
        std::to_string(
            context.windowMode == "fullscreen"
            ? GetMonitorWidth(0)
            : context.screenWidth * context.screenScale
        ) +
        "x" +
        std::to_string(
            context.windowMode == "fullscreen"
            ? GetMonitorHeight(0)
            : context.screenHeight * context.screenScale
        )
    );

    int windowWidth =
        context.screenWidth * context.screenScale;

    int windowHeight =
        context.screenHeight * context.screenScale;

    if (context.windowMode == "fullscreen")
    {
        SetConfigFlags(FLAG_FULLSCREEN_MODE);

        windowWidth =
            GetMonitorWidth(0);

        windowHeight =
            GetMonitorHeight(0);
    }

    InitWindow(
        windowWidth,
        windowHeight,
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
    scriptEngine.setInputSystem(&inputSystem);
    scriptEngine.setFadeSystem(&fadeSystem);
    scriptEngine.setAudioSystem(&audioSystem);

    scriptEngine.setFindObjectFunction(
        [this](const std::string& name)
        {
            return world->findByName(name);
        }
    );

    scriptEngine.setFindObjectByIdFunction(
        [this](const std::string& id)
        {
            return world->findByRuntimeId(id);
        }
    );

    scriptEngine.setSpawnObjectFunction(
        [this](
            RuntimeObject& source,
            const std::string& resourceId
            )
        {
            world->spawn(
                source,
                resourceId,
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
            return world->rayCast(
                source,
                angle,
                distance
            );
        }
    );

    scriptEngine.setKeepOnlyFunction(
        [this](const std::string& runtimeId)
        {
            world->keepOnly(runtimeId);
        }
    );
}

void Engine::update()
{
    const float delta =
        safeFrameDelta();

    scriptEngine.setFrameDelta(delta);

    inputSystem.update(
        delta,
        context.screenWidth,
        context.screenHeight,
        renderDestination(context)
    );

    world->update(
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

    world->draw(
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
        renderDestination(context),
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
    return world->findByName(name);
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
