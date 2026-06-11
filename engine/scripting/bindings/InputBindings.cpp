#include "InputBindings.h"

#include <quickjs.h>
#include <raylib.h>

namespace
{
    JSValue jsKeyDownGeneric(
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

        int key = 0;
        JS_ToInt32(context, &key, argv[0]);

        return JS_NewBool(context, IsKeyDown(key));
    }

    JSValue jsKeyPressedGeneric(
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

        int key = 0;
        JS_ToInt32(context, &key, argv[0]);

        return JS_NewBool(context, IsKeyPressed(key));
    }
}

void InputBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(context, global, "KEY_UP", JS_NewInt32(context, KEY_UP));
    JS_SetPropertyStr(context, global, "KEY_DOWN", JS_NewInt32(context, KEY_DOWN));
    JS_SetPropertyStr(context, global, "KEY_LEFT", JS_NewInt32(context, KEY_LEFT));
    JS_SetPropertyStr(context, global, "KEY_RIGHT", JS_NewInt32(context, KEY_RIGHT));
    JS_SetPropertyStr(context, global, "KEY_SPACE", JS_NewInt32(context, KEY_SPACE));

    JSValue key =
        JS_NewObject(context);

    JS_SetPropertyStr(
        context,
        key,
        "down",
        JS_NewCFunction(context, jsKeyDownGeneric, "down", 1)
    );

    JS_SetPropertyStr(
        context,
        key,
        "pressed",
        JS_NewCFunction(context, jsKeyPressedGeneric, "pressed", 1)
    );

    JS_SetPropertyStr(context, global, "Key", key);

    JS_FreeValue(context, global);
}
