#include "ScriptEngine.h"
#include "../runtime/RuntimeConstants.h"
#include "../debug/Logger.h"
#include "../audio/AudioSystem.h"
#include "../graphics/FadeSystem.h"
#include "../machine/VideoColorProcessor.h"
#include "../tools/ColorParser.h"
#include "ScriptBindings.h"

#include <quickjs.h>
#include <fstream>
#include <sstream>
#include <cmath>
#include <variant>
#include <raylib.h>

namespace
{
    std::string toJsStringLiteral(const std::string& value)
    {
        std::string escaped = "'";

        for (const char ch : value)
        {
            switch (ch)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '\'':
                escaped += "\\'";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
            }
        }

        escaped += "'";

        return escaped;
    }

    enum RuntimeViewProperty
    {
        RuntimeViewId,
        RuntimeViewName,
        RuntimeViewGroup,
        RuntimeViewRole,
        RuntimeViewAlive,
        RuntimeViewVisible,
        RuntimeViewX,
        RuntimeViewY,
        RuntimeViewWidth,
        RuntimeViewHeight,
        RuntimeViewSpeed,
        RuntimeViewAngle,
        RuntimeViewVelocityX,
        RuntimeViewVelocityY,
        RuntimeViewRotationSpeed
    };

    JSValue runtimeViewGetter(
        JSContext* context,
        JSValueConst,
        int,
        JSValueConst*,
        int magic,
        JSValueConst* data
    )
    {
        const char* idText =
            JS_ToCString(context, data[0]);

        const std::string runtimeId =
            idText == nullptr ? "" : idText;

        if (idText != nullptr)
        {
            JS_FreeCString(context, idText);
        }

        if (magic == RuntimeViewId)
        {
            return JS_NewString(context, runtimeId.c_str());
        }

        ScriptEngine* scriptEngine =
            static_cast<ScriptEngine*>(
                JS_GetContextOpaque(context)
            );

        RuntimeObject* object =
            scriptEngine == nullptr || runtimeId.empty()
                ? nullptr
                : scriptEngine->findObjectByRuntimeId(runtimeId);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        switch (magic)
        {
        case RuntimeViewName:
            return JS_NewString(context, object->name.c_str());
        case RuntimeViewGroup:
            return JS_NewString(context, object->group.c_str());
        case RuntimeViewRole:
            return JS_NewString(context, object->role.c_str());
        case RuntimeViewAlive:
            return JS_NewBool(context, object->alive);
        case RuntimeViewVisible:
            return JS_NewBool(context, object->visible);
        case RuntimeViewX:
            return JS_NewFloat64(context, object->position.x);
        case RuntimeViewY:
            return JS_NewFloat64(context, object->position.y);
        case RuntimeViewWidth:
            return JS_NewFloat64(context, object->size.x);
        case RuntimeViewHeight:
            return JS_NewFloat64(context, object->size.y);
        case RuntimeViewSpeed:
            return JS_NewFloat64(context, object->speed);
        case RuntimeViewAngle:
            return JS_NewFloat64(context, object->angle);
        case RuntimeViewVelocityX:
            return JS_NewFloat64(context, object->velocity.x);
        case RuntimeViewVelocityY:
            return JS_NewFloat64(context, object->velocity.y);
        case RuntimeViewRotationSpeed:
            return JS_NewFloat64(context, object->rotationSpeed);
        default:
            return JS_UNDEFINED;
        }
    }

    void defineRuntimeViewGetter(
        JSContext* context,
        JSValueConst object,
        const char* name,
        const std::string& runtimeId,
        RuntimeViewProperty property
    )
    {
        JSValue data[1] = {
            JS_NewString(context, runtimeId.c_str())
        };

        JSValue getter =
            JS_NewCFunctionData(
                context,
                runtimeViewGetter,
                0,
                property,
                1,
                data
            );

        JS_FreeValue(context, data[0]);

        JSAtom atom =
            JS_NewAtom(context, name);

        JS_DefinePropertyGetSet(
            context,
            object,
            atom,
            getter,
            JS_UNDEFINED,
            JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE
        );

        JS_FreeAtom(context, atom);
    }

}

ScriptEngine::ScriptEngine()
{
    runtime = JS_NewRuntime();
    context = JS_NewContext(runtime);

    ScriptBindings::registerAll(
        context,
        this
    );

}

ScriptEngine::~ScriptEngine()
{
    for (auto& pair : scriptModules)
    {
        ScriptModule& module = pair.second;

        JS_FreeValue(context, module.born);
        JS_FreeValue(context, module.action);
        JS_FreeValue(context, module.motion);
        JS_FreeValue(context, module.collision);
        JS_FreeValue(context, module.draw);
        JS_FreeValue(context, module.dead);
    }

    scriptModules.clear();

    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
}

void ScriptEngine::eval(const std::string& code)
{
    JSValue result = JS_Eval(
        context,
        code.c_str(),
        code.size(),
        "<eval>",
        JS_EVAL_TYPE_GLOBAL
    );

    if (JS_IsException(result))
    {
        JSValue exception = JS_GetException(context);
        const char* message = JS_ToCString(context, exception);

        Logger::error("script", message);

        JS_FreeCString(context, message);
        JS_FreeValue(context, exception);
    }

    JS_FreeValue(context, result);
}

void ScriptEngine::callScriptFunction(
    const std::string& script,
    const std::string& function
)
{
    JSValue func =
        getCachedFunction(script, function);

    if (!JS_IsFunction(context, func))
    {
        return;
    }

    JSValue global =
        JS_GetGlobalObject(context);

    JSValue result =
        JS_Call(
            context,
            func,
            global,
            0,
            nullptr
        );

    if (JS_IsException(result))
    {
        JSValue exception =
            JS_GetException(context);

        JSValue stack =
            JS_GetPropertyStr(context, exception, "stack");

        const char* message =
            JS_ToCString(context, exception);

        const char* stackMessage =
            JS_ToCString(context, stack);

        Logger::error(
            "script",
            "Error calling '" + function + "' in script '" + script + "': " +
            std::string(message != nullptr ? message : "JS error") +
            std::string(stackMessage != nullptr ? stackMessage : " undefined")
        );

        JS_FreeCString(context, stackMessage);
        JS_FreeValue(context, stack);
        JS_FreeCString(context, message);
        JS_FreeValue(context, exception);
    }

    JS_FreeValue(context, result);
    JS_FreeValue(context, global);
}

void ScriptEngine::callScriptFunction(
    const std::string& script,
    const std::string& function,
    RuntimeObject& object
)
{
    JSValue func =
        getCachedFunction(script, function);

    if (!JS_IsFunction(context, func))
    {
        return;
    }

    JSValue global =
        JS_GetGlobalObject(context);

    JSValue self =
        createJsObject(object);

    const std::string objectRuntimeId =
        object.runtimeId;

    JSValue args[1] = { self };

    activeScriptObjects[objectRuntimeId] =
        &object;

    JSValue result =
        JS_Call(
            context,
            func,
            global,
            1,
            args
        );

    if (JS_IsException(result))
    {
        JSValue exception =
            JS_GetException(context);

        JSValue stack =
            JS_GetPropertyStr(context, exception, "stack");

        const char* message =
            JS_ToCString(context, exception);

        const char* stackMessage =
            JS_ToCString(context, stack);

        Logger::error(
            "script",
            "Error calling '" + function + "' in script '" + script + "': " +
            std::string(message != nullptr ? message : "JS error") +
            std::string(stackMessage != nullptr ? stackMessage : " undefined")
        );

        JS_FreeCString(context, stackMessage);
        JS_FreeValue(context, stack);
        JS_FreeCString(context, message);
        JS_FreeValue(context, exception);
    }

    activeScriptObjects.erase(objectRuntimeId);

    JS_FreeValue(context, result);
    JS_FreeValue(context, self);
    JS_FreeValue(context, global);
}

void ScriptEngine::callScriptFunction(
    const std::string& script,
    const std::string& function,
    RuntimeObject& object,
    RuntimeObject& other
)
{
    JSValue func =
        getCachedFunction(script, function);

    if (!JS_IsFunction(context, func))
    {
        return;
    }

    JSValue global =
        JS_GetGlobalObject(context);

    JSValue self =
        createJsObject(object);

    JSValue otherObject =
        createJsObject(other);

    const std::string objectRuntimeId =
        object.runtimeId;

    const std::string otherRuntimeId =
        other.runtimeId;

    JSValue args[2] = {
        self,
        otherObject
    };

    activeScriptObjects[objectRuntimeId] =
        &object;
    activeScriptObjects[otherRuntimeId] =
        &other;

    JSValue result =
        JS_Call(
            context,
            func,
            global,
            2,
            args
        );

    if (JS_IsException(result))
    {
        JSValue exception =
            JS_GetException(context);

        JSValue stack =
            JS_GetPropertyStr(context, exception, "stack");

        const char* message =
            JS_ToCString(context, exception);

        const char* stackMessage =
            JS_ToCString(context, stack);

        Logger::error(
            "script",
            "Error calling '" + function + "' in script '" + script + "': " +
            std::string(message != nullptr ? message : "JS error") +
            std::string(stackMessage != nullptr ? stackMessage : " undefined")
        );

        JS_FreeCString(context, stackMessage);
        JS_FreeValue(context, stack);
        JS_FreeCString(context, message);
        JS_FreeValue(context, exception);
    }

    activeScriptObjects.erase(objectRuntimeId);
    activeScriptObjects.erase(otherRuntimeId);

    JS_FreeValue(context, result);
    JS_FreeValue(context, self);
    JS_FreeValue(context, otherObject);
    JS_FreeValue(context, global);
}

void ScriptEngine::loadScript(const std::string& path)
{
    if (loadedScripts.contains(path))
    {
        return;
    }

    std::ifstream file(path);

    if (!file.is_open())
    {
        Logger::error("script", "The script could not be opened: " + path);
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    const std::string code = buffer.str();
    const std::string scriptKey =
        toJsStringLiteral(path);

    const std::string wrapped =
        "globalThis.Flx = globalThis.Flx || {};"
        "Flx.scripts = Flx.scripts || {};"
        "Flx.scripts[" + scriptKey + "] = (function(){"
        + code +
        " return {"
        "born: typeof born === 'function' ? born : undefined,"
        "action: typeof action === 'function' ? action : undefined,"
        "motion: typeof motion === 'function' ? motion : undefined,"
        "collision: typeof collision === 'function' ? collision : undefined,"
        "draw: typeof draw === 'function' ? draw : undefined,"
        "dead: typeof dead === 'function' ? dead : undefined"
        "};"
        "})();";

    eval(wrapped);

    cacheScriptModule(path);

    loadedScripts.insert(path);

    Logger::debug("script", "Loaded script: " + path);
}

void ScriptEngine::loadScript(
    const std::string& id,
    const std::string& code
)
{
    if (loadedScripts.contains(id))
    {
        return;
    }

    const std::string scriptKey =
        toJsStringLiteral(id);

    const std::string wrapped =
        "globalThis.Flx = globalThis.Flx || {};"
        "Flx.scripts = Flx.scripts || {};"
        "Flx.scripts[" + scriptKey + "] = (function(){"
        + code +
        " return {"
        "born: typeof born === 'function' ? born : undefined,"
        "action: typeof action === 'function' ? action : undefined,"
        "motion: typeof motion === 'function' ? motion : undefined,"
        "collision: typeof collision === 'function' ? collision : undefined,"
        "draw: typeof draw === 'function' ? draw : undefined,"
        "dead: typeof dead === 'function' ? dead : undefined"
        "};"
        "})();";

    eval(wrapped);

    cacheScriptModule(id);

    loadedScripts.insert(id);

    Logger::debug("script", "Loaded script: " + id);
}

void ScriptEngine::cacheScriptModule(const std::string& path)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JSValue flx =
        JS_GetPropertyStr(context, global, "Flx");

    JSValue scripts =
        JS_GetPropertyStr(context, flx, "scripts");

    JSValue module =
        JS_GetPropertyStr(context, scripts, path.c_str());

    ScriptModule scriptModule;

    JSValue born =
        JS_GetPropertyStr(context, module, "born");

    JSValue action =
        JS_GetPropertyStr(context, module, "action");

    JSValue motion =
        JS_GetPropertyStr(context, module, "motion");

    JSValue collision =
        JS_GetPropertyStr(context, module, "collision");

    JSValue draw =
        JS_GetPropertyStr(context, module, "draw");

    JSValue dead =
        JS_GetPropertyStr(context, module, "dead");

    scriptModule.born =
        JS_DupValue(context, born);

    scriptModule.action =
        JS_DupValue(context, action);

    scriptModule.motion =
        JS_DupValue(context, motion);

    scriptModule.collision =
        JS_DupValue(context, collision);

    scriptModule.draw =
        JS_DupValue(context, draw);

    scriptModule.dead =
        JS_DupValue(context, dead);

    scriptModules[path] = scriptModule;

    JS_FreeValue(context, born);
    JS_FreeValue(context, action);
    JS_FreeValue(context, motion);
    JS_FreeValue(context, collision);
    JS_FreeValue(context, draw);
    JS_FreeValue(context, dead);

    JS_FreeValue(context, module);
    JS_FreeValue(context, scripts);
    JS_FreeValue(context, flx);
    JS_FreeValue(context, global);
}

JSValue ScriptEngine::getCachedFunction(
    const std::string& script,
    const std::string& function
)
{
    auto it = scriptModules.find(script);

    if (it == scriptModules.end())
    {
        return JS_UNDEFINED;
    }

    ScriptModule& module = it->second;

    if (function == "born")
    {
        return module.born;
    }

    if (function == "action")
    {
        return module.action;
    }

    if (function == "motion")
    {
        return module.motion;
    }

    if (function == "collision")
    {
        return module.collision;
    }

    if (function == "draw")
    {
        return module.draw;
    }

    if (function == "dead")
    {
        return module.dead;
    }

    return JS_UNDEFINED;
}

void ScriptEngine::setFindObjectFunction(
    FindObjectFunction function
)
{
    findObject = function;
}

void ScriptEngine::setFindObjectByIdFunction(
    FindObjectByIdFunction function
)
{
    findObjectById = function;
}

void ScriptEngine::setFindObjectDefinitionFunction(
    FindObjectDefinitionFunction function
)
{
    findObjectDefinitionById = function;
}

const ObjectDefinition* ScriptEngine::findObjectDefinition(
    const std::string& id
) const
{
    if (!findObjectDefinitionById)
    {
        return nullptr;
    }

    return findObjectDefinitionById(id);
}

RuntimeObject* ScriptEngine::findObjectByRuntimeId(
    const std::string& id
)
{
    const auto active =
        activeScriptObjects.find(id);

    if (active != activeScriptObjects.end())
    {
        return active->second;
    }

    if (!findObjectById)
    {
        return nullptr;
    }

    return findObjectById(id);
}

RuntimeObject* ScriptEngine::findObjectByName(
    const std::string& name
)
{
    if (!findObject)
    {
        return nullptr;
    }

    return findObject(name);
}

void ScriptEngine::setSpawnObjectFunction(
    SpawnObjectFunction function
)
{
    spawnObjectFunction = function;
}

void ScriptEngine::spawnObject(
    RuntimeObject& source,
    const std::string& resourceId
)
{
    if (!spawnObjectFunction)
    {
        return;
    }

    spawnObjectFunction(
        source,
        resourceId
    );
}

void ScriptEngine::setRayCastFunction(
    RayCastFunction function
)
{
    rayCastFunction =
        function;
}

RayCastResult ScriptEngine::rayCast(
    RuntimeObject& source,
    float angle,
    float distance
)
{
    if (!rayCastFunction)
    {
        return RayCastResult{};
    }

    return rayCastFunction(
        source,
        angle,
        distance
    );
}

void ScriptEngine::setKeepOnlyFunction(
    KeepOnlyFunction function
)
{
    keepOnlyFunction =
        function;
}

void ScriptEngine::keepOnly(const std::string& runtimeId)
{
    if (!keepOnlyFunction)
    {
        return;
    }

    keepOnlyFunction(runtimeId);
}

void ScriptEngine::setKillObjectFunction(
    ObjectRuntimeFunction function
)
{
    killObjectFunction =
        function;
}

void ScriptEngine::killObject(const std::string& runtimeId)
{
    if (!killObjectFunction)
    {
        return;
    }

    killObjectFunction(runtimeId);
}

void ScriptEngine::setShowObjectFunction(
    ObjectRuntimeFunction function
)
{
    showObjectFunction =
        function;
}

void ScriptEngine::showObject(const std::string& runtimeId)
{
    if (!showObjectFunction)
    {
        return;
    }

    showObjectFunction(runtimeId);
}

void ScriptEngine::setHideObjectFunction(
    ObjectRuntimeFunction function
)
{
    hideObjectFunction =
        function;
}

void ScriptEngine::hideObject(const std::string& runtimeId)
{
    if (!hideObjectFunction)
    {
        return;
    }

    hideObjectFunction(runtimeId);
}

JSValue ScriptEngine::createJsObject(RuntimeObject& object)
{
    JSValue self =
        JS_NewObject(context);

    defineRuntimeViewGetter(context, self, "id", object.runtimeId, RuntimeViewId);
    defineRuntimeViewGetter(context, self, "name", object.runtimeId, RuntimeViewName);
    defineRuntimeViewGetter(context, self, "group", object.runtimeId, RuntimeViewGroup);
    defineRuntimeViewGetter(context, self, "role", object.runtimeId, RuntimeViewRole);
    defineRuntimeViewGetter(context, self, "alive", object.runtimeId, RuntimeViewAlive);
    defineRuntimeViewGetter(context, self, "visible", object.runtimeId, RuntimeViewVisible);
    defineRuntimeViewGetter(context, self, "x", object.runtimeId, RuntimeViewX);
    defineRuntimeViewGetter(context, self, "y", object.runtimeId, RuntimeViewY);
    defineRuntimeViewGetter(context, self, "width", object.runtimeId, RuntimeViewWidth);
    defineRuntimeViewGetter(context, self, "height", object.runtimeId, RuntimeViewHeight);
    defineRuntimeViewGetter(context, self, "speed", object.runtimeId, RuntimeViewSpeed);
    defineRuntimeViewGetter(context, self, "angle", object.runtimeId, RuntimeViewAngle);
    defineRuntimeViewGetter(context, self, "velocityX", object.runtimeId, RuntimeViewVelocityX);
    defineRuntimeViewGetter(context, self, "velocityY", object.runtimeId, RuntimeViewVelocityY);
    defineRuntimeViewGetter(context, self, "rotationSpeed", object.runtimeId, RuntimeViewRotationSpeed);
    JS_PreventExtensions(context, self);

    return self;
}

void ScriptEngine::writeGlobalValue(
    const std::string& key,
    const ScriptValue& value
)
{
    globalState[key] =
        value;
}

std::optional<ScriptValue> ScriptEngine::readGlobalValue(
    const std::string& key
) const
{
    const auto it =
        globalState.find(key);

    if (it == globalState.end())
    {
        return std::nullopt;
    }

    return it->second;
}

void ScriptEngine::setScreenScale(int scale)
{
    screenScale = scale;
}

int ScriptEngine::getScreenScale() const
{
    return screenScale;
}

void ScriptEngine::setVideoChip(const VideoChipDefinition* nextVideoChip)
{
    videoChip =
        nextVideoChip;
}

Color ScriptEngine::parseColor(
    const std::string& color,
    Color fallback
) const
{
    return projectColor(
        ColorParser::parse(
            color,
            fallback
        )
    );
}

Color ScriptEngine::projectColor(Color color) const
{
    if (videoChip == nullptr)
    {
        return color;
    }

    return VideoColorProcessor::project(
        color,
        *videoChip
    );
}

void ScriptEngine::setFrameDelta(float delta)
{
    frameDelta =
        delta;
}

float ScriptEngine::getFrameDelta() const
{
    return frameDelta;
}

void ScriptEngine::setRuntimeFrame(uint64_t frame)
{
    runtimeFrame =
        frame;
}

uint64_t ScriptEngine::getRuntimeFrame() const
{
    return runtimeFrame;
}

void ScriptEngine::setInputSystem(InputSystem* nextInputSystem)
{
    inputSystem =
        nextInputSystem;
}

InputSystem* ScriptEngine::getInputSystem() const
{
    return inputSystem;
}

void ScriptEngine::setFadeSystem(FadeSystem* nextFadeSystem)
{
    fadeSystem = nextFadeSystem;
}

void ScriptEngine::fadeOn(const std::string& color)
{
    if (fadeSystem == nullptr)
    {
        return;
    }

    fadeSystem->fadeOn(
        parseColor(color, BLACK)
    );
}

void ScriptEngine::fadeOff(const std::string& color)
{
    if (fadeSystem == nullptr)
    {
        return;
    }

    fadeSystem->fadeOff(
        parseColor(color, BLACK)
    );
}

void ScriptEngine::fadeSet(
    float alpha,
    const std::string& color
)
{
    if (fadeSystem == nullptr)
    {
        return;
    }

    fadeSystem->set(
        alpha,
        parseColor(color, BLACK)
    );
}

bool ScriptEngine::fadeActive() const
{
    return fadeSystem != nullptr && fadeSystem->isActive();
}

bool ScriptEngine::fadeDone() const
{
    return fadeSystem == nullptr || fadeSystem->isDone();
}

float ScriptEngine::fadeAlpha() const
{
    if (fadeSystem == nullptr)
    {
        return 0.0f;
    }

    return fadeSystem->getAlpha();
}

void ScriptEngine::setAudioSystem(AudioSystem* nextAudioSystem)
{
    audioSystem = nextAudioSystem;
}

void ScriptEngine::playSound(
    RuntimeObject& source,
    const std::string& id
)
{
    if (audioSystem == nullptr)
    {
        return;
    }

    const auto it =
        source.sounds.find(id);

    if (it == source.sounds.end())
    {
        Logger::warning(
            "audio",
            "Sound not found: " + id + " in " + source.runtimeId
        );

        return;
    }

    audioSystem->play(it->second);
}

void ScriptEngine::playMusic(
    RuntimeObject& source,
    const std::string& id
)
{
    if (audioSystem == nullptr)
    {
        return;
    }

    const auto it =
        source.music.find(id);

    if (it == source.music.end())
    {
        Logger::warning(
            "audio",
            "Music not found: " + id + " in " + source.runtimeId
        );

        return;
    }

    audioSystem->playMusic(it->second);
}

void ScriptEngine::stopMusic()
{
    if (audioSystem == nullptr)
    {
        return;
    }

    audioSystem->stopMusic();
}

void ScriptEngine::pauseMusic()
{
    if (audioSystem == nullptr)
    {
        return;
    }

    audioSystem->togglePauseMusic();
}

bool ScriptEngine::musicActive() const
{
    return audioSystem != nullptr && audioSystem->isMusicActive();
}

bool ScriptEngine::musicPaused() const
{
    return audioSystem != nullptr && audioSystem->isMusicPaused();
}

void ScriptEngine::requestExit()
{
    requestedExit = true;

    Logger::info(
        "runtime",
        "Exit requested"
    );
}

bool ScriptEngine::exitRequested() const
{
    return requestedExit;
}

bool ScriptEngine::saveValue(
    const std::string& name,
    const std::string& key,
    const PersistedValue& value
)
{
    return persistenceSystem.save(
        name,
        key,
        value
    );
}

std::optional<PersistedValue> ScriptEngine::loadValue(
    const std::string& name,
    const std::string& key
)
{
    return persistenceSystem.load(
        name,
        key
    );
}
