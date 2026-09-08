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
#include <vector>

namespace
{
    JSValue createRayResult(
        JSContext* context,
        const RayCastResult& result
    )
    {
        if (!result.hit)
        {
            return JS_UNDEFINED;
        }

        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* objectHit =
            scriptEngine == nullptr
            ? nullptr
            : scriptEngine->findObjectByRuntimeId(result.objectId);

        if (objectHit == nullptr || !objectHit->alive)
        {
            return JS_UNDEFINED;
        }

        JSValue object =
            JS_NewObject(context);

        JS_SetPropertyStr(
            context,
            object,
            "object",
            scriptEngine->createRuntimeObjectView(*objectHit)
        );

        JS_SetPropertyStr(
            context,
            object,
            "collider",
            JS_NewString(context, result.collider.c_str())
        );

        JS_SetPropertyStr(context, object, "pointX", JS_NewFloat64(context, result.point.x));
        JS_SetPropertyStr(context, object, "pointY", JS_NewFloat64(context, result.point.y));
        JS_SetPropertyStr(context, object, "normalX", JS_NewFloat64(context, result.normal.x));
        JS_SetPropertyStr(context, object, "normalY", JS_NewFloat64(context, result.normal.y));
        JS_SetPropertyStr(context, object, "distance", JS_NewFloat64(context, result.distance));

        JS_PreventExtensions(context, object);

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

    void warnRuntimeObjectCannotBePersisted()
    {
        Logger::warning(
            "script",
            "Cannot persist a live RuntimeObject. Store its `id` and resolve it with `find_id()` when needed."
        );
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

        RuntimeObject* object =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            Logger::warning(
                "runtime",
                "kill called without a valid object"
            );

            return JS_UNDEFINED;
        }

        scriptEngine->killObject(object->runtimeId);

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

        RuntimeObject* object =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            Logger::warning(
                "runtime",
                "show called without a valid object"
            );

            return JS_UNDEFINED;
        }

        scriptEngine->showObject(object->runtimeId);

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

        RuntimeObject* object =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            Logger::warning(
                "runtime",
                "hide called without a valid object"
            );

            return JS_UNDEFINED;
        }

        scriptEngine->hideObject(object->runtimeId);

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

        RuntimeObject* object =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            Logger::warning(
                "runtime",
                "keep_only called without a valid object"
            );

            return JS_UNDEFINED;
        }

        scriptEngine->keepOnly(object->runtimeId);

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

        RuntimeObject* source =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (source == nullptr)
        {
            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        double angle = 0.0;
        double distance = 0.0;

        JS_ToFloat64(context, &angle, argv[1]);
        JS_ToFloat64(context, &distance, argv[2]);

        return createRayResult(
            context,
            scriptEngine->rayCast(
                *source,
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
            argc < 2 ? nullptr : runtimeObjectViewFromArgument(context, argv[0]);

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
            argc < 3 ? nullptr : runtimeObjectViewFromArgument(context, argv[0]);

        std::string key;
        ScriptValue value;

        if (object == nullptr || !readKeyArgument(context, argv[1], key))
        {
            return JS_UNDEFINED;
        }

        if (isRuntimeObjectView(context, argv[2]))
        {
            warnRuntimeObjectCannotBePersisted();

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

        if (isRuntimeObjectView(context, argv[1]))
        {
            warnRuntimeObjectCannotBePersisted();

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

    JSValue jsColliderSetEnabled(
        JSContext* context,
        int argc,
        JSValueConst* argv,
        bool enabled
    )
    {
        RuntimeObject* object =
            argc < 2 ? nullptr : runtimeObjectViewFromArgument(context, argv[0]);

        std::string colliderName;

        if (object == nullptr ||
            !readKeyArgument(context, argv[1], colliderName))
        {
            return JS_UNDEFINED;
        }

        auto it =
            object->collisions.find(colliderName);

        if (it == object->collisions.end())
        {
            Logger::warning(
                "collision",
                "Unknown collider '" + colliderName + "' in " + object->runtimeId
            );

            return JS_UNDEFINED;
        }

        it->second.enabled =
            enabled;

        return JS_UNDEFINED;
    }

    JSValue jsColliderOn(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return jsColliderSetEnabled(
            context,
            argc,
            argv,
            true
        );
    }

    JSValue jsColliderOff(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return jsColliderSetEnabled(
            context,
            argc,
            argv,
            false
        );
    }

    JSValue jsFindId(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        std::string id;

        if (scriptEngine == nullptr ||
            argc < 1 ||
            !readKeyArgument(context, argv[0], id))
        {
            return JS_UNDEFINED;
        }

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        if (object == nullptr || !object->alive)
        {
            return JS_UNDEFINED;
        }

        return scriptEngine->createRuntimeObjectView(*object);
    }

    JSValue jsFindName(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        std::string name;

        JSValue array =
            JS_NewArray(context);

        if (scriptEngine == nullptr ||
            argc < 1 ||
            !readKeyArgument(context, argv[0], name))
        {
            return array;
        }

        std::vector<RuntimeObject*> matches =
            scriptEngine->findObjectsByName(name);

        uint32_t index = 0;

        for (RuntimeObject* object : matches)
        {
            if (object == nullptr || !object->alive)
            {
                continue;
            }

            JS_SetPropertyUint32(
                context,
                array,
                index++,
                scriptEngine->createRuntimeObjectView(*object)
            );
        }

        return array;
    }

    JSValue jsFindParent(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectViewFromArgument(context, argv[0]);

        if (scriptEngine == nullptr || object == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeObject* parent =
            scriptEngine->findParent(object->runtimeId);

        if (parent == nullptr || !parent->alive)
        {
            return JS_UNDEFINED;
        }

        return scriptEngine->createRuntimeObjectView(*parent);
    }

    JSValue jsFindChildren(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectViewFromArgument(context, argv[0]);

        JSValue array =
            JS_NewArray(context);

        if (scriptEngine == nullptr || object == nullptr)
        {
            return array;
        }

        std::vector<RuntimeObject*> children =
            scriptEngine->findChildren(object->runtimeId);

        uint32_t index = 0;

        for (RuntimeObject* child : children)
        {
            if (child == nullptr || !child->alive)
            {
                continue;
            }

            JS_SetPropertyUint32(
                context,
                array,
                index++,
                scriptEngine->createRuntimeObjectView(*child)
            );
        }

        return array;
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
        "collider_on",
        JS_NewCFunction(context, jsColliderOn, "collider_on", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "collider_off",
        JS_NewCFunction(context, jsColliderOff, "collider_off", 2)
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

    JS_SetPropertyStr(
        context,
        global,
        "find_id",
        JS_NewCFunction(context, jsFindId, "find_id", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "find_name",
        JS_NewCFunction(context, jsFindName, "find_name", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "find_parent",
        JS_NewCFunction(context, jsFindParent, "find_parent", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "find_children",
        JS_NewCFunction(context, jsFindChildren, "find_children", 1)
    );

    JS_FreeValue(context, global);
}
