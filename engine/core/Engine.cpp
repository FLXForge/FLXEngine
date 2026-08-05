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

    int effectiveOutputScale(
        const FlxContext& context,
        const RunOptions& options
    )
    {
        return options.scaleOverride.value_or(
            context.machine.video.outputScale
        );
    }

    bool fullscreenRequested(const RunOptions& options)
    {
        return options.windowMode == WindowMode::Fullscreen;
    }

    Rectangle renderDestination(
        const FlxContext& context,
        const RunOptions& options
    )
    {
        const int screenWidth =
            context.machine.video.screenWidth;

        const int screenHeight =
            context.machine.video.screenHeight;

        const int screenScale =
            effectiveOutputScale(context, options);

        if (!fullscreenRequested(options))
        {
            return Rectangle{
                0.0f,
                0.0f,
                static_cast<float>(screenWidth * screenScale),
                static_cast<float>(screenHeight * screenScale)
            };
        }

        const float windowWidth =
            static_cast<float>(GetScreenWidth());

        const float windowHeight =
            static_cast<float>(GetScreenHeight());

        const float horizontalScale =
            windowWidth / static_cast<float>(screenWidth);

        const float verticalScale =
            windowHeight / static_cast<float>(screenHeight);

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
            static_cast<float>(screenWidth) * scale;

        const float height =
            static_cast<float>(screenHeight) * scale;

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
    RunOptions options;

    if (maxFrames >= 0)
    {
        options.maxFrames = maxFrames;
    }

    run(project, options);
}

void Engine::run(
    const CompiledProject& project,
    const RunOptions& options
)
{
    SetTraceLogCallback(Logger::rayLibLog);

    init(project, options);

    int frameCount = 0;
    const int maxFrames =
        options.maxFrames.value_or(-1);

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

void Engine::init(const CompiledProject& project, const RunOptions& options)
{
    loadProject(project, options);

    SetTargetFPS(60);
}

void Engine::loadProject(const CompiledProject& project, const RunOptions& options)
{
    context =
        project.context;

    runOptions =
        options;

    Logger::setConsoleEnabled(runOptions.debugConsole);
    Logger::setDebugEnabled(runOptions.debugLogs);
    SetTraceLogLevel(
        runOptions.debugConsole
        ? LOG_ALL
        : LOG_NONE
    );

    Logger::info(
        "project",
        "Loaded project: " + context.name
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

    scriptEngine.setScreenScale(1);
    configureScriptEngine();
    world->load(project, scriptEngine);
}

void Engine::initWindow()
{
    const std::string title =
        context.title.empty()
        ? "Flx"
        : context.title;

    const int screenWidth =
        context.machine.video.screenWidth;

    const int screenHeight =
        context.machine.video.screenHeight;

    const int screenScale =
        effectiveOutputScale(context, runOptions);

    Logger::info(
        "graphics",
        "Window size: " +
        std::to_string(
            fullscreenRequested(runOptions)
            ? GetMonitorWidth(0)
            : screenWidth * screenScale
        ) +
        "x" +
        std::to_string(
            fullscreenRequested(runOptions)
            ? GetMonitorHeight(0)
            : screenHeight * screenScale
        )
    );

    int windowWidth =
        screenWidth * screenScale;

    int windowHeight =
        screenHeight * screenScale;

    if (fullscreenRequested(runOptions))
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
        std::to_string(context.machine.video.screenWidth) +
        "x" +
        std::to_string(context.machine.video.screenHeight) +
        " scale " +
        std::to_string(effectiveOutputScale(context, runOptions))
    );

    renderTarget =
        LoadRenderTexture(
            context.machine.video.screenWidth,
            context.machine.video.screenHeight
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
        context.machine.video.screenWidth,
        context.machine.video.screenHeight,
        renderDestination(context, runOptions)
    );

    world->update(
        scriptEngine,
        static_cast<float>(context.machine.video.screenWidth),
        static_cast<float>(context.machine.video.screenHeight),
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
        static_cast<float>(context.machine.video.screenWidth),
        static_cast<float>(context.machine.video.screenHeight),
        runOptions.debugCollisions
    );

    fadeSystem.draw(
        context.machine.video.screenWidth,
        context.machine.video.screenHeight,
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
        renderDestination(context, runOptions),
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
