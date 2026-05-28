#pragma once

#include "../runtime/RuntimeObject.h"
#include "../scripting/ScriptEngine.h"
#include "../project/GameConfig.h"

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>

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

    void actionPhase();
    void motionPhase();
    void collisionPhase();

    void loadObjectsFromJson(const std::string& path);

    using BehaviorFunction =
        std::function<void(RuntimeObject&)>;

    void loadProject(const std::string& flxPath);
    void initWindow();
    void configureScriptEngine();
    void loadScripts();
    void loadPrefabs();

    std::string resolveJsonPath(
        const std::string& basePath,
        const std::string& file
    ) const;

    std::string resolveScriptPath(
        const std::string& basePath,
        const std::string& file
    ) const;

    std::unordered_map<std::string, RuntimeObject> prefabs;
private:

    int screenWidth;
    int screenHeight;
    int screenScale;

    ScriptEngine scriptEngine;
    std::vector<RuntimeObject> objects;
    GameConfig gameConfig;
    std::string projectBasePath;
};