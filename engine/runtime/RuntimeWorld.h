#pragma once

#include "../compiler/CompiledProject.h"
#include "../collision/CollisionDebugFrame.h"
#include "ObjectDefinition.h"
#include "RayCastResult.h"
#include "RuntimeObject.h"

#include <string>
#include <vector>
#include <cstdint>

class ScriptEngine;

struct RuntimeLoadResult
{
    bool success = false;
    Diagnostics diagnostics;
};

struct WorldExtentContribution
{
    Vector2 position{ 0.0f, 0.0f };
    Vector2 size{ 0.0f, 0.0f };
    bool hasSize = false;
};

class RuntimeWorld
{
public:
    RuntimeWorld();

    RuntimeLoadResult load(
        const CompiledProject& project,
        ScriptEngine& scriptEngine
    );

    void update(
        ScriptEngine& scriptEngine,
        float delta
    );

    void setCollisionDebugEnabled(bool enabled);

    const WorldExtent& getWorldExtent() const;

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

    bool creationActive(const RuntimeObject& object) const;
    void kill(const std::string& runtimeId);
    void show(const std::string& runtimeId);
    void hide(const std::string& runtimeId);
    RuntimeObject* findByName(const std::string& name);
    RuntimeObject* findByRuntimeId(const std::string& id);
    RuntimeObject* findLiveByRuntimeId(const std::string& id);
    std::vector<RuntimeObject*> findAllLiveByName(const std::string& name);
    RuntimeObject* findLiveParent(const std::string& runtimeId);
    std::vector<RuntimeObject*> findLiveChildren(const std::string& runtimeId);
    void keepOnly(const std::string& runtimeId);
    RayCastResult rayCast(
        RuntimeObject& source,
        ScriptEngine& scriptEngine,
        float angle,
        float distance
    );

private:
    RuntimeObject createRuntimeObject(
        const ObjectDefinition& definition,
        const std::string& resourceId,
        const std::string& parentId
    );

    RuntimeObject createIndividualChild(
        const RuntimeObject& parent,
        const ObjectDefinition& definition,
        const std::string& resourceId,
        const std::string& childId
    );

    RuntimeObject createGridChild(
        const RuntimeObject& parent,
        const ObjectDefinition& definition,
        const std::string& resourceId,
        const std::string& childId,
        int row,
        int column
    );

    void instantiateAutoChildren(
        RuntimeObject& parent,
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

    void instantiateIteratorChildren(
        RuntimeObject& parent,
        std::vector<RuntimeObject>& target
    );

    size_t iteratorInstanceCount(
        const RuntimeObject& parent,
        const std::vector<RuntimeObject>* target
    ) const;

    void maintainIteratorCreation(ScriptEngine& scriptEngine);

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
    bool flushSpawnQueueForLoad(
        ScriptEngine& scriptEngine,
        Diagnostics& diagnostics
    );

    void beginFrame();

    void actionPhase(ScriptEngine& scriptEngine);

    void motionPhase(
        ScriptEngine& scriptEngine
    );

    void drawPhase(ScriptEngine& scriptEngine);

    void applyAttachments();

    void deadPhase(ScriptEngine& scriptEngine);

    void cleanupDeadObjects();

    void updateObjectTime(float delta);

    void drawCollisionDebug(
        float rasterWidth,
        float rasterHeight
    ) const;

    bool computeWorldExtent(Diagnostics& diagnostics);

    void collectWorldExtentContribution(
        const RuntimeObject& object,
        const ObjectDefinition& definition
    );

    std::string createRuntimeId(const std::string& name);

    int nextRuntimeId;
    uint64_t frameIndex = 0;
    const ResourceRegistry* resources = nullptr;
    std::vector<RuntimeObject> objects;
    std::vector<RuntimeObject> pendingObjects;
    WorldExtent worldExtent;
    std::vector<WorldExtentContribution> worldExtentContributions;
    bool collectingWorldExtentContributions = false;
    CollisionDebugFrame collisionDebugFrame;
    bool collisionDebugEnabled = false;
    bool automaticInstantiationFailed = false;
    std::string automaticInstantiationFailure;
    std::vector<ResourceId> automaticInstantiationStack;
    size_t automaticInstantiationCreated = 0;
};
