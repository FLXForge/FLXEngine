#pragma once

#include "../runtime/RuntimeWorld.h"
#include "../scripting/ScriptEngine.h"
#include "../project/FlxContext.h"
#include "../audio/AudioSystem.h"
#include "../graphics/FadeSystem.h"

#include <string>

class Engine
{
public:

    Engine();

    void run(const std::string& flxPath);
    RuntimeObject* find(const std::string& name);
private:

    void init(const std::string& flxPath);
    void update();
    void draw();
    void shutdown();

    void loadProject(const std::string& flxPath);
    void initWindow();
    void configureScriptEngine();
    float safeFrameDelta() const;
private:
    ScriptEngine scriptEngine;
    RuntimeWorld world;
    AudioSystem audioSystem;
    FadeSystem fadeSystem;
    FlxContext context;
};
