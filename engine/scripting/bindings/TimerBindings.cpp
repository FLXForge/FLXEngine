#include "TimerBindings.h"
#include "BindingHelpers.h"
#include "../../runtime/RuntimeObject.h"

#include <algorithm>
#include <quickjs.h>
#include <string>

namespace
{
    RuntimeObject* objectFromArgument(
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

    const char* timerNameFromArgument(
        JSContext* context,
        JSValueConst value
    )
    {
        return JS_ToCString(context, value);
    }

    JSValue jsTimer(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 3)
        {
            return JS_UNDEFINED;
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        const char* timerName =
            timerNameFromArgument(context, argv[1]);

        double duration = 0.0;

        JS_ToFloat64(context, &duration, argv[2]);

        if (object != nullptr && timerName != nullptr)
        {
            object->timers[timerName].left =
                static_cast<float>(
                    std::max(0.0, duration)
                );
        }

        if (timerName != nullptr)
        {
            JS_FreeCString(context, timerName);
        }

        return JS_UNDEFINED;
    }

    JSValue jsTimerActive(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            return JS_NewBool(context, false);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        const char* timerName =
            timerNameFromArgument(context, argv[1]);

        bool active =
            false;

        if (object != nullptr && timerName != nullptr)
        {
            const auto it =
                object->timers.find(timerName);

            active =
                it != object->timers.end() &&
                it->second.left > 0.0f;
        }

        if (timerName != nullptr)
        {
            JS_FreeCString(context, timerName);
        }

        return JS_NewBool(context, active);
    }

    JSValue jsTimerLeft(
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

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        const char* timerName =
            timerNameFromArgument(context, argv[1]);

        double left =
            0.0;

        if (object != nullptr && timerName != nullptr)
        {
            const auto it =
                object->timers.find(timerName);

            if (it != object->timers.end())
            {
                left =
                    it->second.left;
            }
        }

        if (timerName != nullptr)
        {
            JS_FreeCString(context, timerName);
        }

        return JS_NewFloat64(context, left);
    }

    JSValue jsTimerClear(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            return JS_UNDEFINED;
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        const char* timerName =
            timerNameFromArgument(context, argv[1]);

        if (object != nullptr && timerName != nullptr)
        {
            object->timers.erase(timerName);
        }

        if (timerName != nullptr)
        {
            JS_FreeCString(context, timerName);
        }

        return JS_UNDEFINED;
    }
}

void TimerBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        global,
        "timer",
        JS_NewCFunction(context, jsTimer, "timer", 3)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_active",
        JS_NewCFunction(context, jsTimerActive, "timer_active", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_left",
        JS_NewCFunction(context, jsTimerLeft, "timer_left", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_clear",
        JS_NewCFunction(context, jsTimerClear, "timer_clear", 2)
    );

    JS_FreeValue(context, global);
}
