#include "Engine.h"
#include "../debug/Logger.h"
#include "../machine/VideoColorProcessor.h"
#include "../tools/ColorParser.h"

#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <limits>

Engine::Engine() = default;

namespace
{
    constexpr float MaxFrameDelta =
        1.0f / 30.0f;

    constexpr int HostTargetFps =
        60;

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

EngineResult Engine::run(
    const CompiledProject& project,
    const RunOptions& options
)
{
    EngineResult result;

    if (hasRun)
    {
        result.diagnostics.error(
            DiagnosticCode::EngineAlreadyRun,
            "Engine instance can only run one project",
            "engine"
        );

        result.exitReason =
            EngineExitReason::InitializationFailed;

        return result;
    }

    hasRun = true;
    SetTraceLogCallback(Logger::rayLibLog);

    if (!init(project, options, result))
    {
        shutdown();
        return result;
    }

    const int maxFrames =
        options.maxFrames.value_or(-1);

    while (!WindowShouldClose() && !session.finished())
    {
        update();

        if (session.result().exitReason == FlxSessionExitReason::ScriptRequestedExit)
        {
            break;
        }

        if (session.state() == FlxSessionState::Failed) break;
        draw();
        if (session.state() == FlxSessionState::Failed) break;

        ++result.framesExecuted;

        if (maxFrames >= 0 && result.framesExecuted >= maxFrames)
        {
            result.exitReason =
                EngineExitReason::FrameLimitReached;

            break;
        }
    }

    if (session.result().exitReason == FlxSessionExitReason::ScriptRequestedExit)
    {
        result.exitReason =
            EngineExitReason::ScriptRequestedExit;
    }
    else if (result.exitReason != EngineExitReason::FrameLimitReached)
    {
        result.exitReason =
            EngineExitReason::WindowClosed;
    }

    if (session.state() == FlxSessionState::Failed)
    {
        result.diagnostics.append(session.result().diagnostics);
        result.exitReason = EngineExitReason::RuntimeLoadFailed;
    }
    result.success = !result.diagnostics.hasErrors();

    shutdown();

    return result;
}

bool Engine::init(
    const CompiledProject& project,
    const RunOptions& options,
    EngineResult& result
)
{
    if (!loadProject(
        project,
        options,
        result
    ))
    {
        return false;
    }

    SetTargetFPS(HostTargetFps);

    return true;
}

bool Engine::loadProject(
    const CompiledProject& project,
    const RunOptions& options,
    EngineResult& result
)
{
    context =
        project.context;

    runOptions = options;
    backgroundColor = VideoColorProcessor::project(
        ColorParser::parse(context.machine.video.clearColor, BLACK), context.machine.video);

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

    if (!validateVideoOutput(result))
    {
        result.exitReason =
            EngineExitReason::InitializationFailed;

        return false;
    }

    if (!initWindow(result))
    {
        result.exitReason =
            EngineExitReason::InitializationFailed;

        return false;
    }

    Logger::info(
        "graphics",
        "Logical screen: " +
        std::to_string(context.machine.video.screenWidth) +
        "x" +
        std::to_string(context.machine.video.screenHeight) +
        " scale " +
        std::to_string(effectiveOutputScale(context, runOptions))
    );

    InitAudioDevice();
    audioInitialized = IsAudioDeviceReady();
    FlxSessionOptions sessionOptions;
    sessionOptions.enableAudio = audioInitialized;
    sessionOptions.debugCollisions = runOptions.debugCollisions;
    if (!session.load(project, sessionOptions))
    {
        result.diagnostics.append(session.result().diagnostics);
        result.exitReason = EngineExitReason::RuntimeLoadFailed;
        for (const auto& diagnostic : session.result().diagnostics.all())
        {
            if (diagnostic.code == DiagnosticCode::RenderTargetInitializationFailed ||
                diagnostic.code == DiagnosticCode::WindowInitializationFailed ||
                diagnostic.code == DiagnosticCode::InvalidVideoOutputConfiguration)
                result.exitReason = EngineExitReason::InitializationFailed;
        }
        return false;
    }

    return true;
}

bool Engine::validateVideoOutput(EngineResult& result) const
{
    const int screenWidth =
        context.machine.video.screenWidth;

    const int screenHeight =
        context.machine.video.screenHeight;

    const int screenScale =
        effectiveOutputScale(context, runOptions);

    if (screenWidth <= 0)
    {
        result.diagnostics.error(
            DiagnosticCode::InvalidVideoOutputConfiguration,
            "Video screen width must be greater than zero",
            "engine",
            "video.screen.width"
        );
    }

    if (screenHeight <= 0)
    {
        result.diagnostics.error(
            DiagnosticCode::InvalidVideoOutputConfiguration,
            "Video screen height must be greater than zero",
            "engine",
            "video.screen.height"
        );
    }

    if (screenScale <= 0)
    {
        result.diagnostics.error(
            DiagnosticCode::InvalidVideoOutputConfiguration,
            "Video output scale must be greater than zero",
            "engine",
            "video.output.scale"
        );
    }

    if (!result.diagnostics.hasErrors())
    {
        const int maxInt =
            std::numeric_limits<int>::max();

        if (screenWidth > maxInt / screenScale)
        {
            result.diagnostics.error(
                DiagnosticCode::InvalidVideoOutputConfiguration,
                "Window width overflows integer range",
                "engine",
                "video.screen.width"
            );
        }

        if (screenHeight > maxInt / screenScale)
        {
            result.diagnostics.error(
                DiagnosticCode::InvalidVideoOutputConfiguration,
                "Window height overflows integer range",
                "engine",
                "video.screen.height"
            );
        }
    }

    return !result.diagnostics.hasErrors();
}

bool Engine::initWindow(EngineResult& result)
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

    if (!IsWindowReady())
    {
        result.diagnostics.error(
            DiagnosticCode::WindowInitializationFailed,
            "Window could not be initialized",
            "engine",
            "window"
        );

        return false;
    }

    windowInitialized = true;

    return true;
}

void Engine::update()
{
    session.update(safeFrameDelta());
}

void Engine::draw()
{
    if (!session.render()) return;
    const Texture2D texture = session.output();

    BeginDrawing();

    ClearBackground(backgroundColor);

    DrawTexturePro(
        texture,
        Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(texture.width),
            static_cast<float>(-texture.height)
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
    session.unload();

    if (audioInitialized)
    {
        CloseAudioDevice();
        audioInitialized = false;
    }

    if (windowInitialized)
    {
        CloseWindow();
        windowInitialized = false;
    }

    Logger::setDebugEnabled(false);
    Logger::setConsoleEnabled(false);
    SetTraceLogLevel(LOG_INFO);
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
