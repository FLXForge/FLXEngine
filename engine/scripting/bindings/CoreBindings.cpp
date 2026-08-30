#include "CoreBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../runtime/RuntimeConstants.h"
#include "../../runtime/RayCastResult.h"
#include "../ScriptEngine.h"

#include <quickjs.h>
#include <raylib.h>

#include <optional>
#include <string>
#include <variant>

namespace
{
    JSValue createRayResult(
        JSContext* context,
        const RayCastResult& result
    )
    {
        JSValue object =
            JS_NewObject(context);

        JS_SetPropertyStr(
            context,
            object,
            "hit",
            JS_NewBool(context, result.hit)
        );

        JS_SetPropertyStr(
            context,
            object,
            "group",
            JS_NewString(context, result.group.c_str())
        );

        if (result.hit)
        {
            JS_SetPropertyStr(
                context,
                object,
                "distance",
                JS_NewFloat64(context, result.distance)
            );

            JS_SetPropertyStr(
                context,
                object,
                "x",
                JS_NewFloat64(context, result.point.x)
            );

            JS_SetPropertyStr(
                context,
                object,
                "y",
                JS_NewFloat64(context, result.point.y)
            );
        }

        return object;
    }

    RuntimeObject* runtimeObjectFromArgument(
        JSContext* context,
        JSValueConst value
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr)
        {
            return nullptr;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, value, "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            JS_FreeValue(context, idValue);
            return nullptr;
        }

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return object;
    }

    bool jsValueToScriptValue(
        JSContext* context,
        JSValueConst value,
        ScriptValue& scriptValue
    )
    {
        if (JS_IsBool(value))
        {
            scriptValue =
                JS_ToBool(context, value) != 0;

            return true;
        }

        if (JS_IsNumber(value))
        {
            double number = 0.0;

            if (JS_ToFloat64(context, &number, value) != 0)
            {
                return false;
            }

            scriptValue =
                number;

            return true;
        }

        if (JS_IsString(value))
        {
            const char* text =
                JS_ToCString(context, value);

            if (text == nullptr)
            {
                return false;
            }

            scriptValue =
                std::string(text);

            JS_FreeCString(context, text);

            return true;
        }

        return false;
    }

    JSValue scriptValueToJs(
        JSContext* context,
        const ScriptValue& value
    )
    {
        if (std::holds_alternative<bool>(value))
        {
            return JS_NewBool(
                context,
                std::get<bool>(value)
            );
        }

        if (std::holds_alternative<std::string>(value))
        {
            return JS_NewString(
                context,
                std::get<std::string>(value).c_str()
            );
        }

        return JS_NewFloat64(
            context,
            std::get<double>(value)
        );
    }

    bool readKeyArgument(
        JSContext* context,
        JSValueConst value,
        std::string& key
    )
    {
        const char* text =
            JS_ToCString(context, value);

        if (text == nullptr)
        {
            return false;
        }

        key =
            text;

        JS_FreeCString(context, text);

        return true;
    }

    JSValue consoleLog(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        std::string message;

        for (int i = 0; i < argc; ++i)
        {
            const char* str =
                JS_ToCString(context, argv[i]);

            if (str)
            {
                if (!message.empty())
                {
                    message += " ";
                }

                message += str;
                JS_FreeCString(context, str);
            }
        }

        Logger::info("script", message);

        return JS_UNDEFINED;
    }

    JSValue jsKill(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 1 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            Logger::warning(
                "runtime",
                "kill called without a valid object"
            );

            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->killObject(id);

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        if (object != nullptr)
        {
            refreshRuntimeObjectView(context, argv[0], *object);
        }

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    JSValue jsShow(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 1 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            Logger::warning(
                "runtime",
                "show called without a valid object"
            );

            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->showObject(id);

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        if (object != nullptr)
        {
            refreshRuntimeObjectView(context, argv[0], *object);
        }

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    JSValue jsHide(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 1 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            Logger::warning(
                "runtime",
                "hide called without a valid object"
            );

            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->hideObject(id);

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        if (object != nullptr)
        {
            refreshRuntimeObjectView(context, argv[0], *object);
        }

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    JSValue jsKeepOnly(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 1 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            Logger::warning(
                "runtime",
                "keep_only called without a valid object"
            );

            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->keepOnly(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    JSValue jsDelta(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr)
        {
            return JS_NewFloat64(context, 0.0);
        }

        return JS_NewFloat64(
            context,
            scriptEngine->getFrameDelta()
        );
    }

    JSValue jsProbability(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 1)
        {
            return JS_NewBool(context, false);
        }

        int chance = 0;
        int base = 100;

        JS_ToInt32(context, &chance, argv[0]);

        if (argc >= 2)
        {
            JS_ToInt32(context, &base, argv[1]);
        }

        if (base <= 0 || chance <= 0)
        {
            return JS_NewBool(context, false);
        }

        if (chance >= base)
        {
            return JS_NewBool(context, true);
        }

        const int value =
            GetRandomValue(1, base);

        return JS_NewBool(
            context,
            value <= chance
        );
    }

    JSValue jsRandom(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            return JS_NewFloat64(context, 0.0);
        }

        double min = 0.0;
        double max = 0.0;

        JS_ToFloat64(context, &min, argv[0]);
        JS_ToFloat64(context, &max, argv[1]);

        const double randomValue =
            min +
            static_cast<double>(GetRandomValue(0, 1000000)) /
            1000000.0 *
            (max - min);

        return JS_NewFloat64(
            context,
            randomValue
        );
    }

    JSValue jsRay(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 3)
        {
            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            JS_FreeValue(context, idValue);

            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        RuntimeObject* source =
            scriptEngine->findObjectByRuntimeId(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        if (source == nullptr)
        {
            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        RuntimeObject querySource =
            *source;

        JSValue xValue =
            JS_GetPropertyStr(context, argv[0], "x");

        JSValue yValue =
            JS_GetPropertyStr(context, argv[0], "y");

        double x =
            querySource.position.x;

        double y =
            querySource.position.y;

        JS_ToFloat64(context, &x, xValue);
        JS_ToFloat64(context, &y, yValue);

        querySource.position = Vector2{
            static_cast<float>(x),
            static_cast<float>(y)
        };

        JS_FreeValue(context, xValue);
        JS_FreeValue(context, yValue);

        double angle = 0.0;
        double distance = 0.0;

        JS_ToFloat64(context, &angle, argv[1]);
        JS_ToFloat64(context, &distance, argv[2]);

        return createRayResult(
            context,
            scriptEngine->rayCast(
                querySource,
                static_cast<float>(angle),
                static_cast<float>(distance)
            )
        );
    }

    bool jsValueToPersistedValue(
        JSContext* context,
        JSValueConst value,
        PersistedValue& persisted
    )
    {
        if (JS_IsBool(value))
        {
            persisted.type =
                PersistedValue::Type::Boolean;
            persisted.booleanValue =
                JS_ToBool(context, value) != 0;

            return true;
        }

        if (JS_IsNumber(value))
        {
            persisted.type =
                PersistedValue::Type::Number;

            return JS_ToFloat64(
                context,
                &persisted.numberValue,
                value
            ) == 0;
        }

        if (JS_IsString(value))
        {
            const char* text =
                JS_ToCString(context, value);

            if (text == nullptr)
            {
                return false;
            }

            persisted.type =
                PersistedValue::Type::String;
            persisted.stringValue =
                text;

            JS_FreeCString(context, text);

            return true;
        }

        return false;
    }

    JSValue persistedValueToJs(
        JSContext* context,
        const PersistedValue& value
    )
    {
        if (value.type == PersistedValue::Type::Boolean)
        {
            return JS_NewBool(
                context,
                value.booleanValue
            );
        }

        if (value.type == PersistedValue::Type::String)
        {
            return JS_NewString(
                context,
                value.stringValue.c_str()
            );
        }

        return JS_NewFloat64(
            context,
            value.numberValue
        );
    }

    JSValue jsExit(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine != nullptr)
        {
            scriptEngine->requestExit();
        }

        return JS_UNDEFINED;
    }

    JSValue jsSave(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 3)
        {
            return JS_UNDEFINED;
        }

        const char* name =
            JS_ToCString(context, argv[0]);
        const char* key =
            JS_ToCString(context, argv[1]);

        PersistedValue value;

        const bool validValue =
            jsValueToPersistedValue(
                context,
                argv[2],
                value
            );

        if (name == nullptr || key == nullptr || !validValue)
        {
            Logger::warning(
                "save",
                "save() supports only boolean, number and string values"
            );

            if (name != nullptr)
            {
                JS_FreeCString(context, name);
            }

            if (key != nullptr)
            {
                JS_FreeCString(context, key);
            }

            return JS_UNDEFINED;
        }

        scriptEngine->saveValue(
            name,
            key,
            value
        );

        JS_FreeCString(context, name);
        JS_FreeCString(context, key);

        return JS_UNDEFINED;
    }

    JSValue jsLoad(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 3)
        {
            return JS_UNDEFINED;
        }

        const char* name =
            JS_ToCString(context, argv[0]);
        const char* key =
            JS_ToCString(context, argv[1]);

        PersistedValue defaultValue;

        const bool validDefault =
            jsValueToPersistedValue(
                context,
                argv[2],
                defaultValue
            );

        if (name == nullptr || key == nullptr || !validDefault)
        {
            Logger::warning(
                "save",
                "load() requires a boolean, number or string default value"
            );

            if (name != nullptr)
            {
                JS_FreeCString(context, name);
            }

            if (key != nullptr)
            {
                JS_FreeCString(context, key);
            }

            return validDefault
                ? persistedValueToJs(context, defaultValue)
                : JS_UNDEFINED;
        }

        std::optional<PersistedValue> loaded =
            scriptEngine->loadValue(
                name,
                key
            );

        JS_FreeCString(context, name);
        JS_FreeCString(context, key);

        if (!loaded.has_value())
        {
            return persistedValueToJs(
                context,
                defaultValue
            );
        }

        if (loaded->type != defaultValue.type)
        {
            Logger::warning(
                "save",
                "load() type mismatch; returning default value"
            );

            return persistedValueToJs(
                context,
                defaultValue
            );
        }

        return persistedValueToJs(
            context,
            loaded.value()
        );
    }

    JSValue jsReadLocal(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 2 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        std::string key;

        if (object == nullptr || !readKeyArgument(context, argv[1], key))
        {
            return JS_UNDEFINED;
        }

        const auto it =
            object->local.find(key);

        if (it == object->local.end())
        {
            return JS_UNDEFINED;
        }

        return scriptValueToJs(context, it->second);
    }

    JSValue jsWriteLocal(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 3 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        std::string key;
        ScriptValue value;

        if (object == nullptr || !readKeyArgument(context, argv[1], key))
        {
            return JS_UNDEFINED;
        }

        if (!jsValueToScriptValue(context, argv[2], value))
        {
            Logger::warning(
                "script",
                "write_local() supports only boolean, number and string values"
            );

            return JS_UNDEFINED;
        }

        object->local[key] =
            value;

        return JS_UNDEFINED;
    }

    JSValue jsReadGlobal(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        std::string key;

        if (scriptEngine == nullptr ||
            argc < 1 ||
            !readKeyArgument(context, argv[0], key))
        {
            return JS_UNDEFINED;
        }

        std::optional<ScriptValue> value =
            scriptEngine->readGlobalValue(key);

        if (!value.has_value())
        {
            return JS_UNDEFINED;
        }

        return scriptValueToJs(context, value.value());
    }

    JSValue jsWriteGlobal(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        std::string key;
        ScriptValue value;

        if (scriptEngine == nullptr ||
            argc < 2 ||
            !readKeyArgument(context, argv[0], key))
        {
            return JS_UNDEFINED;
        }

        if (!jsValueToScriptValue(context, argv[1], value))
        {
            Logger::warning(
                "script",
                "write_global() supports only boolean, number and string values"
            );

            return JS_UNDEFINED;
        }

        scriptEngine->writeGlobalValue(
            key,
            value
        );

        return JS_UNDEFINED;
    }
}

void CoreBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JSValue console =
        JS_NewObject(context);

    JS_SetPropertyStr(
        context,
        console,
        "log",
        JS_NewCFunction(context, consoleLog, "log", 1)
    );

    JS_SetPropertyStr(context, global, "console", console);

    JS_SetPropertyStr(context, global, "UP", JS_NewInt32(context, UP));
    JS_SetPropertyStr(context, global, "DOWN", JS_NewInt32(context, DOWN));
    JS_SetPropertyStr(context, global, "LEFT", JS_NewInt32(context, LEFT));
    JS_SetPropertyStr(context, global, "RIGHT", JS_NewInt32(context, RIGHT));
    JS_SetPropertyStr(context, global, "STOP", JS_NewInt32(context, STOP));

    JS_SetPropertyStr(
        context,
        global,
        "kill",
        JS_NewCFunction(context, jsKill, "kill", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "show",
        JS_NewCFunction(context, jsShow, "show", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "hide",
        JS_NewCFunction(context, jsHide, "hide", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "keep_only",
        JS_NewCFunction(context, jsKeepOnly, "keep_only", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "delta",
        JS_NewCFunction(context, jsDelta, "delta", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "random",
        JS_NewCFunction(context, jsRandom, "random", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "probability",
        JS_NewCFunction(context, jsProbability, "probability", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "ray",
        JS_NewCFunction(context, jsRay, "ray", 3)
    );

    JS_SetPropertyStr(
        context,
        global,
        "exit",
        JS_NewCFunction(context, jsExit, "exit", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "save",
        JS_NewCFunction(context, jsSave, "save", 3)
    );

    JS_SetPropertyStr(
        context,
        global,
        "load",
        JS_NewCFunction(context, jsLoad, "load", 3)
    );

    JS_SetPropertyStr(
        context,
        global,
        "read_local",
        JS_NewCFunction(context, jsReadLocal, "read_local", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "write_local",
        JS_NewCFunction(context, jsWriteLocal, "write_local", 3)
    );

    JS_SetPropertyStr(
        context,
        global,
        "read_global",
        JS_NewCFunction(context, jsReadGlobal, "read_global", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "write_global",
        JS_NewCFunction(context, jsWriteGlobal, "write_global", 2)
    );

    JS_FreeValue(context, global);
}
