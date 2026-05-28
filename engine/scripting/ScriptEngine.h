#pragma once

#include "../runtime/RuntimeObject.h"
#include "ScriptModule.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <quickjs.h>

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

    using SpawnObjectFunction =
        std::function<void(
            RuntimeObject& source,
            const SpawnDefinition& spawnDefinition,
            const RuntimeObject& prefab
            )>;

    void setFindObjectFunction(FindObjectFunction function);

    RuntimeObject* findObjectByName(const std::string& name);

    using FindPrefabFunction =
        std::function<RuntimeObject* (const std::string&)>;

    void setFindPrefabFunction(FindPrefabFunction function);
    RuntimeObject* findPrefabByName(const std::string& name);

    FindPrefabFunction findPrefab;

    void setSpawnObjectFunction(SpawnObjectFunction function);

    void spawnObject(
        RuntimeObject& source,
        const SpawnDefinition& spawnDefinition,
        const RuntimeObject& prefab
    );

private:
    JSValue createJsObject(RuntimeObject& object);
    void applyJsObject(RuntimeObject& source, JSValue jsObject);
    void cacheScriptModule(const std::string& path);
    JSValue getCachedFunction(
        const std::string& script,
        const std::string& function
    );
private:
    JSRuntime* runtime;
    JSContext* context;
    std::unordered_set<std::string> loadedScripts;
    std::unordered_map<
        std::string,
        ScriptModule
    > scriptModules;
    SpawnObjectFunction spawnObjectFunction;
    FindObjectFunction findObject;
};