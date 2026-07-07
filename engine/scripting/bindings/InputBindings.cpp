#include "InputBindings.h"
#include "BindingHelpers.h"
#include "../../input/InputSystem.h"

#include <quickjs.h>
#include <cstring>

namespace
{
    InputSystem* inputSystemFromContext(JSContext* context)
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr)
        {
            return nullptr;
        }

        return scriptEngine->getInputSystem();
    }

    int intArgument(
        JSContext* context,
        JSValueConst* argv,
        int index
    )
    {
        int value = 0;
        JS_ToInt32(context, &value, argv[index]);
        return value;
    }

    JSValue jsSystemDown(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input =
            inputSystemFromContext(context);

        if (input == nullptr || argc < 1)
        {
            return JS_NewBool(context, false);
        }

        return JS_NewBool(
            context,
            input->systemDown(intArgument(context, argv, 0))
        );
    }

    JSValue jsSystemPressed(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input =
            inputSystemFromContext(context);

        if (input == nullptr || argc < 1)
        {
            return JS_NewBool(context, false);
        }

        return JS_NewBool(
            context,
            input->systemPressed(intArgument(context, argv, 0))
        );
    }

    JSValue jsPlayerUp(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 1 &&
                input->playerUp(intArgument(context, argv, 0))
        );
    }

    JSValue jsPlayerDown(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 1 &&
                input->playerDown(intArgument(context, argv, 0))
        );
    }

    JSValue jsPlayerLeft(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 1 &&
                input->playerLeft(intArgument(context, argv, 0))
        );
    }

    JSValue jsPlayerRight(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 1 &&
                input->playerRight(intArgument(context, argv, 0))
        );
    }

    JSValue jsPlayerButton(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 2 &&
                input->playerButtonDown(
                    intArgument(context, argv, 0),
                    intArgument(context, argv, 1)
                )
        );
    }

    JSValue jsPlayerPressed(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 2 &&
                input->playerButtonPressed(
                    intArgument(context, argv, 0),
                    intArgument(context, argv, 1)
                )
        );
    }

    JSValue jsPointerX(
        JSContext* context,
        JSValueConst,
        int,
        JSValueConst*
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewFloat64(
            context,
            input == nullptr ? 0.0 : input->pointerX()
        );
    }

    JSValue jsPointerY(
        JSContext* context,
        JSValueConst,
        int,
        JSValueConst*
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewFloat64(
            context,
            input == nullptr ? 0.0 : input->pointerY()
        );
    }

    JSValue jsPointerDown(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 1 &&
                input->pointerDown(intArgument(context, argv, 0))
        );
    }

    JSValue jsPointerPressed(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        InputSystem* input = inputSystemFromContext(context);
        return JS_NewBool(
            context,
            input != nullptr && argc >= 1 &&
                input->pointerPressed(intArgument(context, argv, 0))
        );
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

    void installInputFacade(JSContext* context)
    {
        constexpr const char* source = R"(
globalThis.Input = {
    system: {
        down(buttonIndex) {
            return __flx_input_system_down(buttonIndex);
        },
        pressed(buttonIndex) {
            return __flx_input_system_pressed(buttonIndex);
        }
    },
    player(playerIndex) {
        return {
            up() {
                return __flx_input_player_up(playerIndex);
            },
            down() {
                return __flx_input_player_down(playerIndex);
            },
            left() {
                return __flx_input_player_left(playerIndex);
            },
            right() {
                return __flx_input_player_right(playerIndex);
            },
            button(buttonIndex) {
                return __flx_input_player_button(playerIndex, buttonIndex);
            },
            pressed(buttonIndex) {
                return __flx_input_player_pressed(playerIndex, buttonIndex);
            }
        };
    },
    pointer: {
        x() {
            return __flx_input_pointer_x();
        },
        y() {
            return __flx_input_pointer_y();
        },
        down(buttonIndex) {
            return __flx_input_pointer_down(buttonIndex);
        },
        pressed(buttonIndex) {
            return __flx_input_pointer_pressed(buttonIndex);
        }
    }
};
)";

        JSValue result =
            JS_Eval(
                context,
                source,
                std::strlen(source),
                "flx_input_api",
                JS_EVAL_TYPE_GLOBAL
            );

        JS_FreeValue(context, result);
    }
}

void InputBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    setFunction(context, global, "__flx_input_system_down", jsSystemDown, 1);
    setFunction(context, global, "__flx_input_system_pressed", jsSystemPressed, 1);
    setFunction(context, global, "__flx_input_player_up", jsPlayerUp, 1);
    setFunction(context, global, "__flx_input_player_down", jsPlayerDown, 1);
    setFunction(context, global, "__flx_input_player_left", jsPlayerLeft, 1);
    setFunction(context, global, "__flx_input_player_right", jsPlayerRight, 1);
    setFunction(context, global, "__flx_input_player_button", jsPlayerButton, 2);
    setFunction(context, global, "__flx_input_player_pressed", jsPlayerPressed, 2);
    setFunction(context, global, "__flx_input_pointer_x", jsPointerX, 0);
    setFunction(context, global, "__flx_input_pointer_y", jsPointerY, 0);
    setFunction(context, global, "__flx_input_pointer_down", jsPointerDown, 1);
    setFunction(context, global, "__flx_input_pointer_pressed", jsPointerPressed, 1);

    installInputFacade(context);

    JS_FreeValue(context, global);
}
