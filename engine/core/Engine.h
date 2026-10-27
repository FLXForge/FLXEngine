#pragma once

#include "../compiler/CompiledProject.h"
#include "../runtime/RuntimeWorld.h"
#include "../scripting/ScriptEngine.h"
#include "../core/EngineResult.h"
#include "FlxSession.h"
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

    EngineResult run(const CompiledProject& project, const RunOptions& options);
private:

    bool init(const CompiledProject& project, const RunOptions& options, EngineResult& result);
    void update();
    void draw();
    void shutdown();

    bool loadProject(const CompiledProject& project, const RunOptions& options, EngineResult& result);
    bool validateVideoOutput(EngineResult& result) const;
    bool initWindow(EngineResult& result);
    float safeFrameDelta() const;
private:
    FlxSession session;
    FlxContext context;
    RunOptions runOptions;
    bool windowInitialized = false;
    bool audioInitialized = false;
    bool hasRun = false;
    Color backgroundColor = BLACK;
};
