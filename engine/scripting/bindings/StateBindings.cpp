#include "StateBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
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

    bool transitionAllowed(
        const RuntimeObject& object,
        const std::string& nextState
    )
    {
        if (object.stateTransitions.empty())
        {
            return false;
        }

        if (!object.stateTransitions.contains(nextState))
        {
            return false;
        }

        if (object.state.empty())
        {
            return true;
        }

        const auto it =
            object.stateTransitions.find(object.state);

        if (it == object.stateTransitions.end())
        {
            return false;
        }

        return std::find(
            it->second.begin(),
            it->second.end(),
            nextState
        ) != it->second.end();
    }

    JSValue jsState(
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

        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        const char* stateName =
            JS_ToCString(context, argv[1]);

        if (object == nullptr || stateName == nullptr)
        {
            if (stateName != nullptr)
            {
                JS_FreeCString(context, stateName);
            }

            return JS_UNDEFINED;
        }

        const std::string nextState =
            stateName;

        JS_FreeCString(context, stateName);

        if (object->state == nextState)
        {
            return JS_UNDEFINED;
        }

        if (!transitionAllowed(*object, nextState))
        {
            Logger::warning(
                "state",
                "Invalid transition from '" + object->state +
                "' to '" + nextState + "' in " + object->runtimeId
            );

            return JS_UNDEFINED;
        }

        object->state =
            nextState;

        object->stateTime =
            0.0f;

        object->stateEnteredFrame =
            scriptEngine != nullptr
            ? scriptEngine->getRuntimeFrame()
            : 0;

        return JS_UNDEFINED;
    }

    JSValue jsStateCurrent(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 1)
        {
            return JS_NewString(context, "");
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_NewString(context, "");
        }

        return JS_NewString(
            context,
            object->state.c_str()
        );
    }

    JSValue jsStateActive(
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

        const char* stateName =
            JS_ToCString(context, argv[1]);

        if (object == nullptr || stateName == nullptr)
        {
            if (stateName != nullptr)
            {
                JS_FreeCString(context, stateName);
            }

            return JS_NewBool(context, false);
        }

        const bool active =
            object->state == stateName;

        JS_FreeCString(context, stateName);

        return JS_NewBool(context, active);
    }

    JSValue jsStateEntered(
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
            return JS_NewBool(context, false);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_NewBool(context, false);
        }

        return JS_NewBool(
            context,
            object->stateEnteredFrame == scriptEngine->getRuntimeFrame()
        );
    }

    JSValue jsStateTime(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 1)
        {
            return JS_NewFloat64(context, 0.0);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_NewFloat64(context, 0.0);
        }

        return JS_NewFloat64(
            context,
            object->stateTime
        );
    }
}

void StateBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        global,
        "state",
        JS_NewCFunction(context, jsState, "state", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "state_current",
        JS_NewCFunction(context, jsStateCurrent, "state_current", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "state_active",
        JS_NewCFunction(context, jsStateActive, "state_active", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "state_entered",
        JS_NewCFunction(context, jsStateEntered, "state_entered", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "state_time",
        JS_NewCFunction(context, jsStateTime, "state_time", 1)
    );

    JS_FreeValue(context, global);
}
