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

    JSValue globalObject =
        createGlobalObject();

    exposeGlobalObject(globalObject);

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

    applyGlobalObject(globalObject);

    JS_FreeValue(context, result);
    JS_FreeValue(context, globalObject);
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

    JSValue globalObject =
        createGlobalObject();

    exposeGlobalObject(globalObject);

    JSValue args[1] = { self };

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

    applyJsObject(object, self);
    applyGlobalObject(globalObject);

    JS_FreeValue(context, result);
    JS_FreeValue(context, self);
    JS_FreeValue(context, globalObject);
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

    JSValue globalObject =
        createGlobalObject();

    exposeGlobalObject(globalObject);

    JSValue args[2] = {
        self,
        otherObject
    };

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

    applyJsObject(object, self);
    applyJsObject(other, otherObject);
    applyGlobalObject(globalObject);

    JS_FreeValue(context, result);
    JS_FreeValue(context, self);
    JS_FreeValue(context, otherObject);
    JS_FreeValue(context, globalObject);
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

void ScriptEngine::applyJsObject(
    RuntimeObject& source,
    JSValue jsObject
)
{
    JSValue attachedValue =
        JS_GetPropertyStr(context, jsObject, "attached");

    JSValue xValue =
        JS_GetPropertyStr(context, jsObject, "x");

    JSValue yValue =
        JS_GetPropertyStr(context, jsObject, "y");

    JSValue speedValue =
        JS_GetPropertyStr(context, jsObject, "speed");

    JSValue angleValue =
        JS_GetPropertyStr(context, jsObject, "angle");

    JSValue velocityXValue =
        JS_GetPropertyStr(context, jsObject, "velocityX");

    JSValue velocityYValue =
        JS_GetPropertyStr(context, jsObject, "velocityY");

    JSValue rotationSpeedValue =
        JS_GetPropertyStr(context, jsObject, "rotationSpeed");

    JSValue localValue =
        JS_GetPropertyStr(context, jsObject, "local");

    JSValue widthValue =
        JS_GetPropertyStr(context, jsObject, "width");

    JSValue heightValue =
        JS_GetPropertyStr(context, jsObject, "height");

    JSValue layerValue =
        JS_GetPropertyStr(context, jsObject, "layer");

    if (JS_IsObject(localValue))
    {
        source.local.clear();

        JSPropertyEnum* properties = nullptr;
        uint32_t propertyCount = 0;

        if (JS_GetOwnPropertyNames(
            context,
            &properties,
            &propertyCount,
            localValue,
            JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY
        ) >= 0)
        {
            for (uint32_t i = 0; i < propertyCount; ++i)
            {
                JSAtom atom =
                    properties[i].atom;

                const char* key =
                    JS_AtomToCString(context, atom);

                JSValue value =
                    JS_GetProperty(context, localValue, atom);

                double number = 0.0;

                if (key != nullptr && JS_ToFloat64(context, &number, value) == 0)
                {
                    source.local[key] = number;
                }

                JS_FreeValue(context, value);
                if (key != nullptr)
                {
                    JS_FreeCString(context, key);
                }
                JS_FreeAtom(context, atom);
            }

            js_free(context, properties);
        }
    }

    bool attached = JS_ToBool(context, attachedValue);
    double x = source.position.x;
    double y = source.position.y;
    double speed = source.speed;
    double angle = source.angle;
    double velocityX = source.velocity.x;
    double velocityY = source.velocity.y;
    double rotationSpeed = source.rotationSpeed;
    double width = source.size.x;
    double height = source.size.y;
    int32_t layer = source.layer;

    JS_ToFloat64(context, &x, xValue);
    JS_ToFloat64(context, &y, yValue);
    JS_ToFloat64(context, &speed, speedValue);
    JS_ToFloat64(context, &angle, angleValue);
    JS_ToFloat64(context, &velocityX, velocityXValue);
    JS_ToFloat64(context, &velocityY, velocityYValue);
    JS_ToFloat64(context, &rotationSpeed, rotationSpeedValue);
    JS_ToInt32(context, &layer, layerValue);

    if (JS_ToFloat64(context, &width, widthValue) == 0)
    {
        source.size.x =
            static_cast<float>(width);
    }

    if (JS_ToFloat64(context, &height, heightValue) == 0)
    {
        source.size.y =
            static_cast<float>(height);
    }

    source.attached = attached;
    source.position.x = static_cast<float>(x);
    source.position.y = static_cast<float>(y);
    source.speed = static_cast<float>(speed);
    source.angle = static_cast<float>(angle);
    const Vector2 previousVelocity =
        source.velocity;

    source.velocity.x = static_cast<float>(velocityX);
    source.velocity.y = static_cast<float>(velocityY);

    if (std::abs(source.velocity.x - previousVelocity.x) > 0.00001f ||
        std::abs(source.velocity.y - previousVelocity.y) > 0.00001f)
    {
        source.motionCommanded = true;
    }
    source.rotationSpeed = static_cast<float>(rotationSpeed);
    source.layer = layer;

    JS_FreeValue(context, localValue);
    JS_FreeValue(context, attachedValue);
    JS_FreeValue(context, xValue);
    JS_FreeValue(context, yValue);
    JS_FreeValue(context, speedValue);
    JS_FreeValue(context, angleValue);
    JS_FreeValue(context, velocityXValue);
    JS_FreeValue(context, velocityYValue);
    JS_FreeValue(context, rotationSpeedValue);
    JS_FreeValue(context, widthValue);
    JS_FreeValue(context, heightValue);
    JS_FreeValue(context, layerValue);
}

void ScriptEngine::applyGlobalObject(JSValue globalObject)
{
    if (!JS_IsObject(globalObject))
    {
        return;
    }

    globalState.clear();

    JSPropertyEnum* properties = nullptr;
    uint32_t propertyCount = 0;

    if (JS_GetOwnPropertyNames(
        context,
        &properties,
        &propertyCount,
        globalObject,
        JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY
    ) < 0)
    {
        return;
    }

    for (uint32_t i = 0; i < propertyCount; ++i)
    {
        JSAtom atom =
            properties[i].atom;

        const char* key =
            JS_AtomToCString(context, atom);

        JSValue value =
            JS_GetProperty(context, globalObject, atom);

        double number = 0.0;

        if (
            key != nullptr &&
            JS_ToFloat64(context, &number, value) == 0
            )
        {
            globalState[key] = number;
        }

        JS_FreeValue(context, value);

        if (key != nullptr)
        {
            JS_FreeCString(context, key);
        }

        JS_FreeAtom(context, atom);
    }

    js_free(context, properties);
}

void ScriptEngine::exposeGlobalObject(JSValue globalObject)
{
    JSValue jsGlobal =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        jsGlobal,
        "global",
        JS_DupValue(context, globalObject)
    );

    JS_FreeValue(context, jsGlobal);
}

JSValue ScriptEngine::createGlobalObject()
{
    JSValue globalObject =
        JS_NewObject(context);

    for (const auto& entry : globalState)
    {
        JS_SetPropertyStr(
            context,
            globalObject,
            entry.first.c_str(),
            JS_NewFloat64(
                context,
                entry.second
            )
        );
    }

    return globalObject;
}

JSValue ScriptEngine::createJsObject(RuntimeObject& object)
{
    JSValue self =
        JS_NewObject(context);

    JSValue local =
        JS_NewObject(context);

    for (const auto& pair : object.local)
    {
        JS_SetPropertyStr(
            context,
            local,
            pair.first.c_str(),
            JS_NewFloat64(context, pair.second)
        );
    }

    JS_SetPropertyStr(
        context,
        self,
        "local",
        local
    );

    JS_SetPropertyStr(
        context,
        self,
        "name",
        JS_NewString(context, object.name.c_str())
    );

    JS_SetPropertyStr(
        context,
        self,
        "id",
        JS_NewString(context, object.runtimeId.c_str())
    );

    JS_SetPropertyStr(
        context,
        self,
        "alive",
        JS_NewBool(context, object.alive)
    );

    JS_SetPropertyStr(
        context,
        self,
        "visible",
        JS_NewBool(context, object.visible)
    );

    JS_SetPropertyStr(
        context,
        self,
        "attached",
        JS_NewBool(context, object.attached)
    );

    JS_SetPropertyStr(
        context,
        self,
        "group",
        JS_NewString(context, object.group.c_str())
    );

    JS_SetPropertyStr(
        context,
        self,
        "role",
        JS_NewString(context, object.role.c_str())
    );

    JS_SetPropertyStr(
        context,
        self,
        "controlPlayer",
        JS_NewInt32(context, object.controlPlayer)
    );

    JS_SetPropertyStr(
        context,
        self,
        "layer",
        JS_NewInt32(context, object.layer)
    );

    JS_SetPropertyStr(
        context,
        self,
        "x",
        JS_NewFloat64(context, object.position.x)
    );

    JS_SetPropertyStr(
        context,
        self,
        "previousX",
        JS_NewFloat64(context, object.previousPosition.x)
    );

    JS_SetPropertyStr(
        context,
        self,
        "y",
        JS_NewFloat64(context, object.position.y)
    );

    JS_SetPropertyStr(
        context,
        self,
        "previousY",
        JS_NewFloat64(context, object.previousPosition.y)
    );

    JS_SetPropertyStr(
        context,
        self,
        "width",
        JS_NewFloat64(context, object.size.x)
    );

    JS_SetPropertyStr(
        context,
        self,
        "height",
        JS_NewFloat64(context, object.size.y)
    );

    JS_SetPropertyStr(
        context,
        self,
        "speed",
        JS_NewFloat64(context, object.speed)
    );

    JS_SetPropertyStr(
        context,
        self,
        "angle",
        JS_NewFloat64(context, object.angle)
    );

    JS_SetPropertyStr(
        context,
        self,
        "originX",
        JS_NewFloat64(context, object.origin.x)
    );

    JS_SetPropertyStr(
        context,
        self,
        "originY",
        JS_NewFloat64(context, object.origin.y)
    );

    JS_SetPropertyStr(
        context,
        self,
        "originSpeed",
        JS_NewFloat64(context, object.originSpeed)
    );

    JS_SetPropertyStr(
        context,
        self,
        "rotationSpeed",
        JS_NewFloat64(context, object.rotationSpeed)
    );

    JS_SetPropertyStr(
        context,
        self,
        "velocityX",
        JS_NewFloat64(context, object.velocity.x)
    );

    JS_SetPropertyStr(
        context,
        self,
        "velocityY",
        JS_NewFloat64(context, object.velocity.y)
    );

    return self;
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
