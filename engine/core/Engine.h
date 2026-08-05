#pragma once

#include "../compiler/CompiledProject.h"
#include "../runtime/RuntimeWorld.h"
#include "../scripting/ScriptEngine.h"
#include "../core/RunOptions.h"
#include "../audio/AudioSystem.h"
#include "../graphics/FadeSystem.h"
#include "../input/InputSystem.h"

#include <raylib.h>
#include <memory>
#include <string>

class Engine
{
public:

    Engine();

    void run(const CompiledProject& project, int maxFrames = -1);
    void run(const CompiledProject& project, const RunOptions& options);
    RuntimeObject* find(const std::string& name);
private:

    void init(const CompiledProject& project, const RunOptions& options);
    void update();
    void draw();
    void shutdown();

    void loadProject(const CompiledProject& project, const RunOptions& options);
    void initWindow();
    void initVideoOutput();
    void configureScriptEngine();
    void shutdownVideoOutput();
    float safeFrameDelta() const;
private:
    std::unique_ptr<RuntimeWorld> world;
    ScriptEngine scriptEngine;
    AudioSystem audioSystem;
    InputSystem inputSystem;
    FadeSystem fadeSystem;
    FlxContext context;
    RunOptions runOptions;
    RenderTexture2D renderTarget = {};
    bool renderTargetLoaded = false;
    Color backgroundColor = BLACK;
};
