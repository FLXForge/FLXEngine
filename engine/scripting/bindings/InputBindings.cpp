#include "InputBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../input/InputSystem.h"
#include "../../runtime/RuntimeObject.h"

#include <quickjs.h>

#include <string>

namespace
{
    constexpr const char* SubjectKindProperty = "__flxInputSubject";
    constexpr const char* ControlKindProperty = "__flxInputControl";

    InputSystem* inputSystemFromContext(JSContext* context)
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        return scriptEngine == nullptr
            ? nullptr
            : scriptEngine->getInputSystem();
    }

    int intArgument(
        JSContext* context,
        JSValueConst value,
        int fallback = 0
    )
    {
        int result = fallback;
        JS_ToInt32(context, &result, value);
        return result;
    }

    std::string stringProperty(
        JSContext* context,
        JSValueConst value,
        const char* property
    )
    {
        JSValue jsProperty =
            JS_GetPropertyStr(context, value, property);

        const char* text =
            JS_ToCString(context, jsProperty);

        std::string result =
            text == nullptr ? "" : text;

        if (text != nullptr)
        {
            JS_FreeCString(context, text);
        }

        JS_FreeValue(context, jsProperty);

        return result;
    }

    int intProperty(
        JSContext* context,
        JSValueConst value,
        const char* property,
        int fallback = 0
    )
    {
        JSValue jsProperty =
            JS_GetPropertyStr(context, value, property);

        int result =
            intArgument(context, jsProperty, fallback);

        JS_FreeValue(context, jsProperty);

        return result;
    }

    JSValue makeDescriptor(
        JSContext* context,
        const char* markerName,
        const char* markerValue,
        int index
    )
    {
        JSValue object =
            JS_NewObject(context);

        JS_SetPropertyStr(
            context,
            object,
            markerName,
            JS_NewString(context, markerValue)
        );

        JS_SetPropertyStr(
            context,
            object,
            "index",
            JS_NewInt32(context, index)
        );

        return object;
    }

    JSValue jsButton(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return makeDescriptor(
            context,
            ControlKindProperty,
            "button",
            argc < 1 ? 0 : intArgument(context, argv[0])
        );
    }

    JSValue jsDirection(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return makeDescriptor(
            context,
            ControlKindProperty,
            "direction",
            argc < 1 ? 0 : intArgument(context, argv[0])
        );
    }

    JSValue jsPlayer(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return makeDescriptor(
            context,
            SubjectKindProperty,
            "player",
            argc < 1 ? 1 : intArgument(context, argv[0], 1)
        );
    }

    JSValue jsSystem(
        JSContext* context,
        JSValueConst,
        int,
        JSValueConst*
    )
    {
        return makeDescriptor(
            context,
            SubjectKindProperty,
            "system",
            0
        );
    }

    InputComponent componentFromArgument(
        JSContext* context,
        JSValueConst value
    )
    {
        const int raw =
            intArgument(context, value, 0);

        switch (raw)
        {
            case -1: return InputComponent::Negative;
            case 1: return InputComponent::Positive;
            case -100: return InputComponent::Up;
            case 100: return InputComponent::Down;
            case -101: return InputComponent::Left;
            case 101: return InputComponent::Right;
            default: return InputComponent::Neutral;
        }
    }

    bool readSubject(
        JSContext* context,
        JSValueConst value,
        InputSubject& subject
    )
    {
        if (!JS_IsObject(value))
        {
            return false;
        }

        const std::string kind =
            stringProperty(context, value, SubjectKindProperty);

        if (kind == "player")
        {
            subject.kind = InputSubjectKind::Player;
            subject.index = intProperty(context, value, "index", 1);
            return true;
        }

        if (kind == "system")
        {
            subject.kind = InputSubjectKind::System;
            subject.index = 0;
            return true;
        }

        JSValue controlValue =
            JS_GetPropertyStr(context, value, "controlPlayer");

        int controlPlayer = 0;
        JS_ToInt32(context, &controlPlayer, controlValue);
        JS_FreeValue(context, controlValue);

        if (controlPlayer <= 0)
        {
            Logger::warning(
                "input",
                "RuntimeObject does not declare control.player"
            );

            return false;
        }

        subject.kind = InputSubjectKind::Player;
        subject.index = controlPlayer;
        return true;
    }

    bool readControl(
        JSContext* context,
        JSValueConst value,
        std::string& kind,
        int& index
    )
    {
        if (!JS_IsObject(value))
        {
            return false;
        }

        kind =
            stringProperty(context, value, ControlKindProperty);

        index =
            intProperty(context, value, "index", 0);

        return kind == "button" || kind == "direction";
    }

    enum class InputQuery
    {
        Pressed,
        Down,
        Released
    };

    enum class InputAxis
    {
        Horizontal,
        Vertical
    };

    bool queryButton(
        const InputSystem& input,
        InputQuery query,
        const InputSubject& subject,
        int button
    )
    {
        if (subject.kind == InputSubjectKind::System)
        {
            if (!input.validSystemButton(button))
            {
                Logger::warning("input", "System button outside Input Chip limit");
                return false;
            }

            if (query == InputQuery::Pressed)
            {
                return input.systemButtonPressed(button);
            }

            if (query == InputQuery::Released)
            {
                return input.systemButtonReleased(button);
            }

            return input.systemButtonDown(button);
        }

        if (!input.validPlayer(subject.index))
        {
            Logger::warning("input", "Player outside Input Chip players");
            return false;
        }

        if (!input.validPlayerButton(button))
        {
            Logger::warning("input", "Player button outside Input Chip limit");
            return false;
        }

        if (query == InputQuery::Pressed)
        {
            return input.playerButtonPressed(subject.index, button);
        }

        if (query == InputQuery::Released)
        {
            return input.playerButtonReleased(subject.index, button);
        }

        return input.playerButtonDown(subject.index, button);
    }

    bool queryDirection(
        const InputSystem& input,
        InputQuery query,
        const InputSubject& subject,
        int direction,
        InputComponent component
    )
    {
        if (subject.kind == InputSubjectKind::System)
        {
            Logger::warning("input", "System subject does not support directions");
            return false;
        }

        if (!input.validPlayer(subject.index))
        {
            Logger::warning("input", "Player outside Input Chip players");
            return false;
        }

        if (!input.validDirection(direction))
        {
            Logger::warning("input", "Direction outside Input Chip limit");
            return false;
        }

        if (!input.componentAllowed(direction, component))
        {
            Logger::warning("input", "Direction component is incompatible with Input Chip");
            return false;
        }

        if (query == InputQuery::Pressed)
        {
            return input.playerDirectionPressed(subject.index, direction, component);
        }

        if (query == InputQuery::Released)
        {
            return input.playerDirectionReleased(subject.index, direction, component);
        }

        return input.playerDirectionDown(subject.index, direction, component);
    }

    JSValue jsInputQuery(
        JSContext* context,
        int argc,
        JSValueConst* argv,
        InputQuery query
    )
    {
        InputSystem* input =
            inputSystemFromContext(context);

        if (input == nullptr || argc < 2)
        {
            return JS_NewBool(context, false);
        }

        InputSubject subject;

        if (!readSubject(context, argv[0], subject))
        {
            return JS_NewBool(context, false);
        }

        std::string controlKind;
        int controlIndex = 0;

        if (!readControl(context, argv[1], controlKind, controlIndex))
        {
            Logger::warning("input", "Invalid input control descriptor");
            return JS_NewBool(context, false);
        }

        if (controlKind == "button")
        {
            return JS_NewBool(
                context,
                queryButton(*input, query, subject, controlIndex)
            );
        }

        if (argc < 3)
        {
            Logger::warning("input", "Direction query requires a component");
            return JS_NewBool(context, false);
        }

        return JS_NewBool(
            context,
            queryDirection(
                *input,
                query,
                subject,
                controlIndex,
                componentFromArgument(context, argv[2])
            )
        );
    }

    JSValue jsInputPressed(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return jsInputQuery(context, argc, argv, InputQuery::Pressed);
    }

    JSValue jsInputDown(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return jsInputQuery(context, argc, argv, InputQuery::Down);
    }

    JSValue jsInputReleased(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        return jsInputQuery(context, argc, argv, InputQuery::Released);
    }

    bool directionDown(
        const InputSystem& input,
        const InputSubject& subject,
        int direction,
        InputComponent component
    )
    {
        if (!input.validPlayer(subject.index) ||
            !input.validDirection(direction) ||
            !input.componentAllowed(direction, component))
        {
            return false;
        }

        return queryDirection(
            input,
            InputQuery::Down,
            subject,
            direction,
            component
        );
    }

    JSValue jsInputDirection(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input =
            inputSystemFromContext(context);

        if (input == nullptr || argc < 3)
        {
            return JS_NewInt32(context, 0);
        }

        InputSubject subject;

        if (!readSubject(context, argv[0], subject))
        {
            return JS_NewInt32(context, 0);
        }

        if (subject.kind == InputSubjectKind::System)
        {
            Logger::warning("input", "System subject does not support directions");
            return JS_NewInt32(context, 0);
        }

        std::string controlKind;
        int directionIndex = 0;

        if (!readControl(context, argv[1], controlKind, directionIndex) ||
            controlKind != "direction")
        {
            Logger::warning("input", "input_direction requires a direction descriptor");
            return JS_NewInt32(context, 0);
        }

        const int axisValue =
            intArgument(context, argv[2], 0);

        const InputAxis axis =
            axisValue == 1
                ? InputAxis::Vertical
                : InputAxis::Horizontal;

        if (axis == InputAxis::Horizontal)
        {
            const bool positive =
                directionDown(*input, subject, directionIndex, InputComponent::Right) ||
                directionDown(*input, subject, directionIndex, InputComponent::Positive);

            const bool negative =
                directionDown(*input, subject, directionIndex, InputComponent::Left) ||
                directionDown(*input, subject, directionIndex, InputComponent::Negative);

            if (positive == negative)
            {
                return JS_NewInt32(context, 0);
            }

            return JS_NewInt32(context, positive ? 1 : -1);
        }

        const bool positive =
            directionDown(*input, subject, directionIndex, InputComponent::Up) ||
            directionDown(*input, subject, directionIndex, InputComponent::Positive);

        const bool negative =
            directionDown(*input, subject, directionIndex, InputComponent::Down) ||
            directionDown(*input, subject, directionIndex, InputComponent::Negative);

        if (positive == negative)
        {
            return JS_NewInt32(context, 0);
        }

        return JS_NewInt32(context, positive ? 1 : -1);
    }

    void setFunction(
        JSContext* context,
        JSValue global,
        const char* name,
        JSCFunction* function,
        int length
    )
    {
        JS_SetPropertyStr(
            context,
            global,
            name,
            JS_NewCFunction(context, function, name, length)
        );
    }
}

void InputBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    setFunction(context, global, "button", jsButton, 1);
    setFunction(context, global, "direction", jsDirection, 1);
    setFunction(context, global, "player", jsPlayer, 1);
    setFunction(context, global, "system", jsSystem, 0);
    setFunction(context, global, "input_pressed", jsInputPressed, 2);
    setFunction(context, global, "input_down", jsInputDown, 2);
    setFunction(context, global, "input_released", jsInputReleased, 2);
    setFunction(context, global, "input_direction", jsInputDirection, 3);

    JS_SetPropertyStr(context, global, "NEGATIVE", JS_NewInt32(context, -1));
    JS_SetPropertyStr(context, global, "POSITIVE", JS_NewInt32(context, 1));
    JS_SetPropertyStr(context, global, "HORIZONTAL", JS_NewInt32(context, 0));
    JS_SetPropertyStr(context, global, "VERTICAL", JS_NewInt32(context, 1));

    JS_FreeValue(context, global);
}
