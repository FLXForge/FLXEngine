#include "../support/TestSupport.h"
#include "SessionTestProject.h"
#include <core/FlxSession.h>
#include <core/Engine.h>
#include <audio/AudioSystem.h>
#include <debug/Logger.h>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace flx::test;
namespace
{
    int sentinels = 0;
    void hostLog(int, const char* text, va_list)
    {
        if (std::strstr(text, "host-sentinel")) ++sentinels;
    }
    bool sameColor(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }
    CompiledProject deviceProject(const std::string& color, int width)
    {
        auto project = sessionTestProject(
            "let count=0; function born(o){play_music(o,'theme');play_sound(o,'beep');}"
            "function action(o){if(++count===3)exit();}");
        project.context.machine.video.clearColor = color;
        project.context.machine.video.screenWidth = width;
        auto root = *project.resources.findObject("root");
        MusicDefinition music;
        music.loop = true;
        MusicChannelDefinition channel;
        channel.notes = { "C4", "G4" };
        music.channels.push_back(channel);
        root.music["theme"] = music;
        SoundDefinition sound; sound.duration = 2;
        root.sounds["beep"] = sound;
        // Registry intentionally rejects duplicate resources; replace with a fresh registry.
        ResourceRegistry resources;
        require(resources.addObject("root", root), "device root");
        require(resources.addScript("entry", *project.resources.findScript("entry")), "device script");
        project.resources = std::move(resources);
        return project;
    }
    void verifySurface(const FlxSession& session, Color expected, RenderTexture2D hostTarget)
    {
        const auto texture = session.output();
        require(texture.id != 0, "borrowed FLX surface exists");
        Image logical = LoadImageFromTexture(texture);
        require(sameColor(GetImageColor(logical, 0, 0), expected), "logical Machine background color");
        UnloadImage(logical);
        BeginTextureMode(hostTarget);
        ClearBackground(LIME);
        DrawTexturePro(texture, { 0, 0, static_cast<float>(texture.width), -static_cast<float>(texture.height) },
            { 8, 8, static_cast<float>(texture.width), static_cast<float>(texture.height) }, { 0, 0 }, 0, WHITE);
        EndTextureMode();
        Image presented = LoadImageFromTexture(hostTarget.texture);
        ImageFlipVertical(&presented);
        require(sameColor(GetImageColor(presented, 0, 0), LIME), "host owns the surrounding presentation");
        require(sameColor(GetImageColor(presented, 12, 12), expected), "FLX output composes inside host surface");
        UnloadImage(presented);
        BeginDrawing();
        ClearBackground(LIME);
        DrawTexturePro(hostTarget.texture, { 0, 0, 96, -96 }, { 0, 0, 96, 96 }, { 0, 0 }, 0, WHITE);
        EndDrawing();
    }
}
int main()
{
    if (isHeadlessTestEnvironment()) { std::cout << "[SKIP] persistent host test requires graphics and audio\n"; return 77; }
    SetTraceLogLevel(LOG_WARNING);
    SetTraceLogCallback(hostLog);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(96, 96, "FLX persistent host test");
    if (!IsWindowReady()) return 77;
    InitAudioDevice();
    if (!IsAudioDeviceReady()) { CloseWindow(); return 77; }
    const auto handle = GetWindowHandle();
    auto hostTarget = LoadRenderTexture(96, 96);
    try
    {
        std::ostringstream loggerOutput;
        Logger::setConsoleEnabled(true);
        Logger::setDebugEnabled(true);
        // Explicit sentinels verify session lifetime does not reset host logging.
        TraceLog(LOG_WARNING, "host-sentinel-before");
        FlxSession session;
        for (const auto& entry : { std::pair<std::string, Color>{ "red", RED }, { "blue", BLUE } })
        {
            const int width = entry.first == "red" ? 32 : 48;
            {
                auto source = deviceProject(entry.first, width);
                require(session.load(source), "load in persistent host");
            }
            require(session.output().width == width, "Machine logical resolution");
            require(session.testAudioSystem()->isMusicActive(), "session music plays on host device");
            require(session.testAudioSystem()->testActiveSoundCount() == 1, "session owns active sound");
            int presentations = 0;
            while (!session.finished() && presentations < 5)
            {
                require(session.update(0.01f), "persistent host update");
                if (!session.finished())
                {
                    require(session.render(), "persistent host logical render");
                    verifySurface(session, entry.second, hostTarget);
                    ++presentations;
                }
            }
            require(session.finished() && presentations == 2, "project exits after actual update/presentation cycles");
            require(session.result().exitReason == FlxSessionExitReason::ScriptRequestedExit && session.result().framesExecuted == 2, "script exit and frame count");
            require(!session.testAudioSystem()->isMusicActive() && session.testAudioSystem()->testActiveSoundCount() == 0, "finish releases project playback");
            session.unload(); session.unload();
            require(session.output().id == 0, "unload invalidates borrowed surface");
            require(IsWindowReady() && GetWindowHandle() == handle && IsAudioDeviceReady(), "same host devices survive sequential projects");
        }
        require(session.load(sessionTestProject("function draw(o){exit();write_global('afterDraw',7);}")), "load draw-exit project");
        require(session.render() && session.finished(), "exit from draw completes logical rendering safely");
        const auto afterDraw = session.testGlobalValue("afterDraw");
        require(afterDraw && std::get<double>(*afterDraw) == 7 && session.output().id != 0, "draw callback and final surface survive exit request");
        session.unload();
        auto invalid = deviceProject("red", 32); invalid.rootId = "missing";
        require(!session.load(invalid) && session.state() == FlxSessionState::Failed, "partial load fails after acquiring graphics/audio resources");
        require(session.output().id == 0 && IsWindowReady() && IsAudioDeviceReady(), "partial error cleans owned resources, preserves devices");
        session.unload();
        {
            FlxSession active;
            require(active.load(deviceProject("blue", 48)), "new session after failure");
            require(active.render(), "render without first updating");
        }
        require(IsWindowReady() && IsAudioDeviceReady(), "destroying active session preserves host devices");
        auto before = std::cout.rdbuf(loggerOutput.rdbuf());
        Logger::debug("host", "host-logger-still-enabled");
        std::cout.rdbuf(before);
        require(loggerOutput.str().find("host-logger-still-enabled") != std::string::npos, "session must not reset logger configuration");
        TraceLog(LOG_WARNING, "host-sentinel-after");
        require(sentinels == 2, "session must not reset raylib logging callback/level");
        UnloadRenderTexture(hostTarget); hostTarget = {};
        CloseAudioDevice(); CloseWindow();

        // The standalone host uses exactly the same runtime with its own devices.
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        Engine engine;
        RunOptions options; options.maxFrames = 5;
        const auto standalone = engine.run(deviceProject("red", 32), options);
        require(standalone.success && standalone.exitReason == EngineExitReason::ScriptRequestedExit && standalone.framesExecuted == 2,
            "standalone and embedded agree on completion and rendered frame count");
        require(!IsWindowReady() && !IsAudioDeviceReady(), "standalone still closes its own devices");
        SetTraceLogCallback(nullptr);
        std::cout << "[PASS] two sequential projects in one live graphics/audio host, composition, recovery, teardown and standalone equivalence\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n';
        if (hostTarget.id) UnloadRenderTexture(hostTarget);
        CloseAudioDevice(); CloseWindow(); SetTraceLogCallback(nullptr); return 1;
    }
}
