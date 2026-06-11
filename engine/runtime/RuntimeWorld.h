#pragma once

#include "ObjectDefinition.h"
#include "RuntimeObject.h"

#include <string>
#include <vector>

class ScriptEngine;

class RuntimeWorld
{
public:
    RuntimeWorld();

    void load(
        const ObjectDefinition& rootDefinition,
        ScriptEngine& scriptEngine
    );

    void update(
        ScriptEngine& scriptEngine,
        float screenWidth,
        float screenHeight
    );

    void draw(
        ScriptEngine& scriptEngine,
        int screenScale,
        float screenWidth,
        float screenHeight,
        bool debugCollisions
    );

    void spawn(
        RuntimeObject& source,
        const ObjectDefinition& definition,
        ScriptEngine& scriptEngine
    );

    RuntimeObject* findByName(const std::string& name);
    RuntimeObject* findByRuntimeId(const std::string& id);

private:
    RuntimeObject createRuntimeObject(
        const ObjectDefinition& definition,
        const std::string& parentId
    );

    void instantiateAutoChildren(
        const RuntimeObject& parent,
        std::vector<RuntimeObject>& target
    );

    void loadScriptsForObject(
        RuntimeObject& object,
        ScriptEngine& scriptEngine
    );

    void bornObject(
        RuntimeObject& object,
        ScriptEngine& scriptEngine
    );

    void flushSpawnQueue(ScriptEngine& scriptEngine);

    void actionPhase(ScriptEngine& scriptEngine);

    void motionPhase(
        ScriptEngine& scriptEngine,
        float screenWidth,
        float screenHeight
    );

    void drawPhase(ScriptEngine& scriptEngine);

    void deadPhase(ScriptEngine& scriptEngine);

    void cleanupDeadObjects();

    std::string createRuntimeId(const std::string& name);

    int nextRuntimeId;
    std::vector<RuntimeObject> objects;
    std::vector<RuntimeObject> pendingObjects;
};
