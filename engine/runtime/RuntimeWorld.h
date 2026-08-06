#pragma once

#include "../compiler/CompiledProject.h"
#include "ObjectDefinition.h"
#include "RayCastResult.h"
#include "RuntimeObject.h"

#include <string>
#include <vector>
#include <cstdint>

class ScriptEngine;

class RuntimeWorld
{
public:
    RuntimeWorld();

    void load(
        const CompiledProject& project,
        ScriptEngine& scriptEngine
    );

    void update(
        ScriptEngine& scriptEngine,
        float screenWidth,
        float screenHeight,
        float delta
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
        const std::string& resourceId,
        ScriptEngine& scriptEngine
    );

    RuntimeObject* findByName(const std::string& name);
    RuntimeObject* findByRuntimeId(const std::string& id);
    void keepOnly(const std::string& runtimeId);
    const Diagnostics& loadDiagnostics() const;
    RayCastResult rayCast(
        const RuntimeObject& source,
        float angle,
        float distance
    ) const;

private:
    RuntimeObject createRuntimeObject(
        const ObjectDefinition& definition,
        const std::string& parentId
    );

    RuntimeObject createIndividualChild(
        const RuntimeObject& parent,
        const ObjectDefinition& definition
    );

    RuntimeObject createGridChild(
        const RuntimeObject& parent,
        const ObjectDefinition& definition,
        int row,
        int column
    );

    void instantiateAutoChildren(
        const RuntimeObject& parent,
        std::vector<RuntimeObject>& target
    );

    void instantiateIndividualAutoChildren(
        const RuntimeObject& parent,
        std::vector<RuntimeObject>& target
    );

    void instantiateGridChildren(
        const RuntimeObject& parent,
        const std::string& requestedChildId,
        const std::string& requestedSpawnMode,
        std::vector<RuntimeObject>& target
    );

    bool gridChildIdAt(
        const RuntimeObject& parent,
        int row,
        int column,
        std::string& childId
    ) const;

    bool validGridCreation(
        const RuntimeObject& parent
    ) const;

    void loadScriptsForObject(
        RuntimeObject& object,
        ScriptEngine& scriptEngine
    );

    void bornObject(
        RuntimeObject& object,
        ScriptEngine& scriptEngine
    );

    void flushSpawnQueue(ScriptEngine& scriptEngine);

    void beginFrame();

    void actionPhase(ScriptEngine& scriptEngine);

    void motionPhase(
        ScriptEngine& scriptEngine,
        float screenWidth,
        float screenHeight
    );

    void drawPhase(ScriptEngine& scriptEngine);

    void applyAttachments();

    void deadPhase(ScriptEngine& scriptEngine);

    void cleanupDeadObjects();

    void updateObjectTime(float delta);

    std::string createRuntimeId(const std::string& name);

    int nextRuntimeId;
    uint64_t frameIndex = 0;
    const ResourceRegistry* resources = nullptr;
    Diagnostics lastLoadDiagnostics;
    std::vector<RuntimeObject> objects;
    std::vector<RuntimeObject> pendingObjects;
};
