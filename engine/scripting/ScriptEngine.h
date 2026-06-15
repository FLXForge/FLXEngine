#pragma once

#include "../runtime/RuntimeObject.h"
#include "../runtime/ObjectDefinition.h"
#include "../runtime/RayCastResult.h"
#include "ScriptModule.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <quickjs.h>

class FadeSystem;
class AudioSystem;
struct JSRuntime;
struct JSContext;

class ScriptEngine
{
public:
    ScriptEngine();
    ~ScriptEngine();

    void eval(const std::string& code);
    void loadScript(const std::string& path);

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

    using SpawnObjectFunction =
        std::function<void(
            RuntimeObject& source,
            const ObjectDefinition& definition
            )>;

    using RayCastFunction =
        std::function<RayCastResult(
            RuntimeObject& source,
            float angle,
            float distance
            )>;

    using KeepOnlyFunction =
        std::function<void(const std::string&)>;

    void setFindObjectFunction(FindObjectFunction function);

    RuntimeObject* findObjectByName(const std::string& name);

    void setSpawnObjectFunction(SpawnObjectFunction function);

    void spawnObject(
        RuntimeObject& source,
        const ObjectDefinition& definition
    );

    void setRayCastFunction(RayCastFunction function);

    RayCastResult rayCast(
        RuntimeObject& source,
        float angle,
        float distance
    );

    void setKeepOnlyFunction(KeepOnlyFunction function);

    void keepOnly(const std::string& runtimeId);

    void setFindObjectByIdFunction(
        FindObjectByIdFunction function
    );

    RuntimeObject* findObjectByRuntimeId(
        const std::string& id
    );

    void setScreenScale(int scale);
    int getScreenScale() const;

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

private:
    JSValue createJsObject(RuntimeObject& object);
    void applyJsObject(RuntimeObject& source, JSValue jsObject);
    JSValue createGlobalObject();
    void applyGlobalObject(JSValue globalObject);
    void exposeGlobalObject(JSValue globalObject);
    void cacheScriptModule(const std::string& path);
    JSValue getCachedFunction(
        const std::string& script,
        const std::string& function
    );
private:
    int screenScale = 0;
    JSRuntime* runtime;
    JSContext* context;
    std::unordered_map<std::string, double> globalState;
    std::unordered_set<std::string> loadedScripts;
    std::unordered_map<
        std::string,
        ScriptModule
    > scriptModules;
    std::string keepOnlyRuntimeId;
    SpawnObjectFunction spawnObjectFunction;
    RayCastFunction rayCastFunction;
    KeepOnlyFunction keepOnlyFunction;
    FindObjectFunction findObject;
    FindObjectByIdFunction findObjectById;
    FadeSystem* fadeSystem = nullptr;
    AudioSystem* audioSystem = nullptr;
};
