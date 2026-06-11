#include "DrawBindings.h"
#include "BindingHelpers.h"

#include <quickjs.h>
#include <raylib.h>

namespace
{
    JSValue jsDrawText(
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

        int fontSize = 10;

        if (argc >= 4)
        {
            JS_ToInt32(context, &fontSize, argv[3]);
        }

        const int scale =
            scriptEngine->getScreenScale();

        double x = 0.0;
        double y = 0.0;

        JS_ToFloat64(context, &x, argv[0]);
        JS_ToFloat64(context, &y, argv[1]);

        const char* text =
            JS_ToCString(context, argv[2]);

        if (text == nullptr)
        {
            return JS_UNDEFINED;
        }

        DrawText(
            text,
            static_cast<int>(x * scale),
            static_cast<int>(y * scale),
            fontSize * scale,
            WHITE
        );

        JS_FreeCString(context, text);

        return JS_UNDEFINED;
    }
}

void DrawBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        global,
        "draw_text",
        JS_NewCFunction(context, jsDrawText, "draw_text", 3)
    );

    JS_FreeValue(context, global);
}
