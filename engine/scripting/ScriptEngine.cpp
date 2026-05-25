#include "ScriptEngine.h"
#include "../runtime/RuntimeConstants.h"
#include "../debug/Logger.h"
#include "ScriptBindings.h"

#include <quickjs.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <raylib.h>

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

        JS_FreeValue(context, module.start);
        JS_FreeValue(context, module.action);
        JS_FreeValue(context, module.motion);
        JS_FreeValue(context, module.collision);
        JS_FreeValue(context, module.draw);
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

        const char* message =
            JS_ToCString(context, exception);

        Logger::error(
            "script",
            message != nullptr ? message : "Unknown JS error"
        );

        JS_FreeCString(context, message);
        JS_FreeValue(context, exception);
    }

    applyJsObject(object, self);

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

        const char* message =
            JS_ToCString(context, exception);

        Logger::error(
            "script",
            message != nullptr ? message : "Unknown JS error"
        );

        JS_FreeCString(context, message);
        JS_FreeValue(context, exception);
    }

    applyJsObject(object, self);

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

    const std::string wrapped =
        "globalThis.Flx = globalThis.Flx || {};"
        "Flx.scripts = Flx.scripts || {};"
        "Flx.scripts[" + std::string("'") + path + "'] = (function(){"
        + code +
        " return {"
        "start: typeof start === 'function' ? start : undefined,"
        "action: typeof action === 'function' ? action : undefined,"
        "motion: typeof motion === 'function' ? motion : undefined,"
        "collision: typeof collision === 'function' ? collision : undefined,"
        "draw: typeof draw === 'function' ? draw : undefined"
        "};"
        "})();";

    eval(wrapped);

    cacheScriptModule(path);

    loadedScripts.insert(path);

    Logger::info("script", "Loaded script: " + path);
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

    JSValue start =
        JS_GetPropertyStr(context, module, "start");

    JSValue action =
        JS_GetPropertyStr(context, module, "action");

    JSValue motion =
        JS_GetPropertyStr(context, module, "motion");

    JSValue collision =
        JS_GetPropertyStr(context, module, "collision");

    JSValue draw =
        JS_GetPropertyStr(context, module, "draw");

    scriptModule.start =
        JS_DupValue(context, start);

    scriptModule.action =
        JS_DupValue(context, action);

    scriptModule.motion =
        JS_DupValue(context, motion);

    scriptModule.collision =
        JS_DupValue(context, collision);

    scriptModule.draw =
        JS_DupValue(context, draw);

    scriptModules[path] = scriptModule;

    JS_FreeValue(context, start);
    JS_FreeValue(context, action);
    JS_FreeValue(context, motion);
    JS_FreeValue(context, collision);
    JS_FreeValue(context, draw);

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

    if (function == "start")
    {
        return module.start;
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

    return JS_UNDEFINED;
}

void ScriptEngine::setFindObjectFunction(
    FindObjectFunction function
)
{
    findObject = function;
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

void ScriptEngine::applyJsObject(
    RuntimeObject& source,
    JSValue jsObject
)
{
    JSValue xValue =
        JS_GetPropertyStr(context, jsObject, "x");

    JSValue yValue =
        JS_GetPropertyStr(context, jsObject, "y");

    JSValue speedValue =
        JS_GetPropertyStr(context, jsObject, "speed");

    JSValue angleValue =
        JS_GetPropertyStr(context, jsObject, "angle");

    double x = source.position.x;
    double y = source.position.y;
    double speed = source.speed;
    double angle = source.angle;

    JS_ToFloat64(context, &x, xValue);
    JS_ToFloat64(context, &y, yValue);
    JS_ToFloat64(context, &speed, speedValue);
    JS_ToFloat64(context, &angle, angleValue);

    source.position.x = static_cast<float>(x);
    source.position.y = static_cast<float>(y);
    source.speed = static_cast<float>(speed);
    source.angle = static_cast<float>(angle);

    JS_FreeValue(context, xValue);
    JS_FreeValue(context, yValue);
    JS_FreeValue(context, speedValue);
    JS_FreeValue(context, angleValue);
}

JSValue ScriptEngine::createJsObject(RuntimeObject& object)
{
    JSValue self =
        JS_NewObject(context);

    JS_SetPropertyStr(
        context,
        self,
        "name",
        JS_NewString(context, object.name.c_str())
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
        "x",
        JS_NewFloat64(context, object.position.x)
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

    return self;
}