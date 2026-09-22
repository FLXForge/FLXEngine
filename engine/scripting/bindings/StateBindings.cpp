#include "StateBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../runtime/ObjectDefinition.h"
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
        return runtimeObjectViewFromArgument(context, value);
    }

    bool hasStateMachine(const ObjectDefinition* definition)
    {
        return definition != nullptr &&
            !definition->initialState.empty() &&
            !definition->stateTransitions.empty();
    }

    bool transitionAllowed(
        const ObjectDefinition& definition,
        const std::string& currentState,
        const std::string& nextState
    )
    {
        if (!definition.stateTransitions.contains(nextState))
        {
            return false;
        }

        if (currentState.empty())
        {
            return false;
        }

        const auto it =
            definition.stateTransitions.find(currentState);

        if (it == definition.stateTransitions.end())
        {
            return false;
        }

        return std::find(
            it->second.begin(),
            it->second.end(),
            nextState
        ) != it->second.end();
    }

    JSValue jsStateTo(
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

        RuntimeObject* stateObject =
            scriptEngine == nullptr
            ? nullptr
            : scriptEngine->effectiveStateObject(*object);

        const ObjectDefinition* definition =
            stateObject == nullptr
            ? nullptr
            : scriptEngine->findObjectDefinition(stateObject->definitionId);

        if (!hasStateMachine(definition))
        {
            Logger::warning(
                "state",
                "Object '" + object->runtimeId +
                "' does not define a state machine"
            );

            return JS_UNDEFINED;
        }

        if (!transitionAllowed(*definition, stateObject->state, nextState))
        {
            Logger::warning(
                "state",
                "Invalid transition from '" + stateObject->state +
                "' to '" + nextState + "' in " + object->runtimeId
            );

            return JS_UNDEFINED;
        }

        stateObject->state =
            nextState;

        stateObject->stateTime =
            0.0f;

        stateObject->stateEnteredFrame =
            scriptEngine != nullptr
            ? scriptEngine->getRuntimeFrame() + 1
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

        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* stateObject =
            object == nullptr || scriptEngine == nullptr
            ? nullptr
            : scriptEngine->effectiveStateObject(*object);

        if (stateObject == nullptr ||
            !scriptEngine->hasStateMachine(*stateObject))
        {
            return JS_NewString(context, "");
        }

        return JS_NewString(
            context,
            stateObject->state.c_str()
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

        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* stateObject =
            object == nullptr || scriptEngine == nullptr
            ? nullptr
            : scriptEngine->effectiveStateObject(*object);

        const bool active =
            stateObject != nullptr &&
            scriptEngine->hasStateMachine(*stateObject) &&
            stateObject->state == stateName;

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

        RuntimeObject* stateObject =
            object == nullptr
            ? nullptr
            : scriptEngine->effectiveStateObject(*object);

        if (stateObject == nullptr ||
            !scriptEngine->hasStateMachine(*stateObject))
        {
            return JS_NewBool(context, false);
        }

        return JS_NewBool(
            context,
            stateObject->stateEnteredFrame == scriptEngine->getRuntimeFrame()
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

        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        RuntimeObject* stateObject =
            object == nullptr || scriptEngine == nullptr
            ? nullptr
            : scriptEngine->effectiveStateObject(*object);

        if (stateObject == nullptr ||
            !scriptEngine->hasStateMachine(*stateObject))
        {
            return JS_NewFloat64(context, 0.0);
        }

        return JS_NewFloat64(
            context,
            stateObject->stateTime
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
        "state_to",
        JS_NewCFunction(context, jsStateTo, "state_to", 2)
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
