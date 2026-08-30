#pragma once

#include "../runtime/RuntimeObject.h"
#include "../runtime/ObjectDefinition.h"
#include "../runtime/ScriptValue.h"
#include "../runtime/RayCastResult.h"
#include "../machine/MachineDefinition.h"
#include "../persistence/PersistenceSystem.h"
#include "ScriptModule.h"

#include <raylib.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <cstdint>
#include <optional>
#include <quickjs.h>

class FadeSystem;
class AudioSystem;
class InputSystem;
struct JSRuntime;
struct JSContext;

class ScriptEngine
{
public:
    ScriptEngine();
    ~ScriptEngine();

    void eval(const std::string& code);
    void loadScript(const std::string& path);
    void loadScript(
        const std::string& id,
        const std::string& code
    );

    void callScriptFunction(
        const std::string& script,
        const std::string& function
    );

    void callScriptFunction(
        const std::string& script,
        const std::string& function,
        RuntimeObject& object
    );

    void callScriptFunction(
        const std::string& script,
        const std::string& function,
        RuntimeObject& object,
        RuntimeObject& other
    );

    using FindObjectFunction =
        std::function<RuntimeObject* (const std::string&)>;

    using FindObjectByIdFunction =
        std::function<RuntimeObject* (const std::string&)>;

    using FindObjectDefinitionFunction =
        std::function<const ObjectDefinition* (const std::string&)>;

    using SpawnObjectFunction =
        std::function<void(
            RuntimeObject& source,
            const std::string& resourceId
            )>;

    using RayCastFunction =
        std::function<RayCastResult(
            RuntimeObject& source,
            float angle,
            float distance
            )>;

    using KeepOnlyFunction =
        std::function<void(const std::string&)>;

    using ObjectRuntimeFunction =
        std::function<void(const std::string&)>;

    void setFindObjectFunction(FindObjectFunction function);

    RuntimeObject* findObjectByName(const std::string& name);

    void setSpawnObjectFunction(SpawnObjectFunction function);

    void spawnObject(
        RuntimeObject& source,
        const std::string& resourceId
    );

    void setRayCastFunction(RayCastFunction function);

    RayCastResult rayCast(
        RuntimeObject& source,
        float angle,
        float distance
    );

    void setKeepOnlyFunction(KeepOnlyFunction function);

    void keepOnly(const std::string& runtimeId);

    void setKillObjectFunction(ObjectRuntimeFunction function);
    void killObject(const std::string& runtimeId);

    void setShowObjectFunction(ObjectRuntimeFunction function);
    void showObject(const std::string& runtimeId);

    void setHideObjectFunction(ObjectRuntimeFunction function);
    void hideObject(const std::string& runtimeId);

    void setFindObjectByIdFunction(
        FindObjectByIdFunction function
    );

    RuntimeObject* findObjectByRuntimeId(
        const std::string& id
    );

    void setFindObjectDefinitionFunction(
        FindObjectDefinitionFunction function
    );

    const ObjectDefinition* findObjectDefinition(
        const std::string& id
    ) const;

    void setScreenScale(int scale);
    int getScreenScale() const;
    void setVideoChip(const VideoChipDefinition* videoChip);
    Color parseColor(
        const std::string& color,
        Color fallback
    ) const;
    Color projectColor(Color color) const;
    void setFrameDelta(float delta);
    float getFrameDelta() const;
    void setRuntimeFrame(uint64_t frame);
    uint64_t getRuntimeFrame() const;

    void setInputSystem(InputSystem* inputSystem);
    InputSystem* getInputSystem() const;

    void setFadeSystem(FadeSystem* fadeSystem);
    void fadeOn(const std::string& color);
    void fadeOff(const std::string& color);
    void fadeSet(
        float alpha,
        const std::string& color
    );
    bool fadeActive() const;
    bool fadeDone() const;
    float fadeAlpha() const;

    void setAudioSystem(AudioSystem* audioSystem);
    void playSound(
        RuntimeObject& source,
        const std::string& id
    );
    void playMusic(
        RuntimeObject& source,
        const std::string& id
    );
    void stopMusic();
    void pauseMusic();
    bool musicActive() const;
    bool musicPaused() const;

    void requestExit();
    bool exitRequested() const;

    bool saveValue(
        const std::string& name,
        const std::string& key,
        const PersistedValue& value
    );

    std::optional<PersistedValue> loadValue(
        const std::string& name,
        const std::string& key
    );

    void writeGlobalValue(
        const std::string& key,
        const ScriptValue& value
    );

    std::optional<ScriptValue> readGlobalValue(
        const std::string& key
    ) const;

private:
    JSValue createJsObject(RuntimeObject& object);
    void cacheScriptModule(const std::string& path);
    JSValue getCachedFunction(
        const std::string& script,
        const std::string& function
    );
private:
    int screenScale = 0;
    float frameDelta = 1.0f / 60.0f;
    uint64_t runtimeFrame = 0;
    const VideoChipDefinition* videoChip = nullptr;
    JSRuntime* runtime;
    JSContext* context;
    std::unordered_map<std::string, ScriptValue> globalState;
    std::unordered_map<std::string, RuntimeObject*> activeScriptObjects;
    std::unordered_set<std::string> loadedScripts;
    std::unordered_map<
        std::string,
        ScriptModule
    > scriptModules;
    ObjectRuntimeFunction killObjectFunction;
    ObjectRuntimeFunction showObjectFunction;
    ObjectRuntimeFunction hideObjectFunction;
    SpawnObjectFunction spawnObjectFunction;
    RayCastFunction rayCastFunction;
    KeepOnlyFunction keepOnlyFunction;
    FindObjectFunction findObject;
    FindObjectByIdFunction findObjectById;
    FindObjectDefinitionFunction findObjectDefinitionById;
    FadeSystem* fadeSystem = nullptr;
    AudioSystem* audioSystem = nullptr;
    InputSystem* inputSystem = nullptr;
    PersistenceSystem persistenceSystem;
    bool requestedExit = false;
};
