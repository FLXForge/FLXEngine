#pragma once

#include "../runtime/RuntimeObject.h"
#include "../runtime/ObjectDefinition.h"
#include "../scripting/ScriptEngine.h"
#include "../project/FlxContext.h"

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
    void drawPhase();
    void deadPhase();
    void cleanupDeadObjects();

    void loadProject(const std::string& flxPath);
    void initWindow();
    void bornObject(RuntimeObject& object);
    void configureScriptEngine();
    void loadScripts();
    void loadScriptsForObject(RuntimeObject& object);
    void flushSpawnQueue();
    void instantiateAutoChildren(
        const RuntimeObject& parent,
        std::vector<RuntimeObject>& target
    );

    RuntimeObject createRuntimeObject(
        const ObjectDefinition& definition,
        const std::string& parentId
    );

    std::string createRuntimeId(
        const std::string& name
    );

    RuntimeObject* findByRuntimeId(
        const std::string& id
    );

    std::vector<RuntimeObject> pendingObjects;
private:

    int nextRuntimeId;

    ScriptEngine scriptEngine;
    ObjectDefinition rootDefinition;
    std::vector<RuntimeObject> objects;
    FlxContext context;
};
