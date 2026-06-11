#include "DrawBindings.h"
#include "BindingHelpers.h"
#include "../../tools/ColorParser.h"

#include <quickjs.h>
#include <raylib.h>

#include <string>

namespace
{
    std::string optionalString(
        JSContext* context,
        int argc,
        JSValueConst* argv,
        int index,
        const std::string& fallback
    )
    {
        if (argc <= index)
        {
            return fallback;
        }

        const char* value =
            JS_ToCString(context, argv[index]);

        if (value == nullptr)
        {
            return fallback;
        }

        std::string result =
            value;

        JS_FreeCString(context, value);

        return result;
    }

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

        Color color =
            WHITE;

        if (argc >= 5)
        {
            const char* colorValue =
                JS_ToCString(context, argv[4]);

            if (colorValue != nullptr)
            {
                color =
                    ColorParser::parse(colorValue, WHITE);

                JS_FreeCString(context, colorValue);
            }
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
            color
        );

        JS_FreeCString(context, text);

        return JS_UNDEFINED;
    }

    JSValue jsFadeOn(
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
            return JS_UNDEFINED;
        }

        const std::string color =
            optionalString(context, argc, argv, 0, "black");

        scriptEngine->fadeOn(color);

        return JS_UNDEFINED;
    }

    JSValue jsFadeOff(
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
            return JS_UNDEFINED;
        }

        const std::string color =
            optionalString(context, argc, argv, 0, "black");

        scriptEngine->fadeOff(color);

        return JS_UNDEFINED;
    }

    JSValue jsFadeSet(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 1)
        {
            return JS_UNDEFINED;
        }

        double alpha = 0.0;
        JS_ToFloat64(context, &alpha, argv[0]);

        const std::string color =
            optionalString(context, argc, argv, 1, "black");

        scriptEngine->fadeSet(
            static_cast<float>(alpha),
            color
        );

        return JS_UNDEFINED;
    }

    JSValue jsFadeActive(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        return JS_NewBool(
            context,
            scriptEngine != nullptr && scriptEngine->fadeActive()
        );
    }

    JSValue jsFadeDone(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        return JS_NewBool(
            context,
            scriptEngine == nullptr || scriptEngine->fadeDone()
        );
    }

    JSValue jsFadeAlpha(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        const double alpha =
            scriptEngine == nullptr
            ? 0.0
            : scriptEngine->fadeAlpha();

        return JS_NewFloat64(context, alpha);
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
        JS_NewCFunction(context, jsDrawText, "draw_text", 5)
    );

    JS_SetPropertyStr(
        context,
        global,
        "fade_on",
        JS_NewCFunction(context, jsFadeOn, "fade_on", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "fade_off",
        JS_NewCFunction(context, jsFadeOff, "fade_off", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "fade_set",
        JS_NewCFunction(context, jsFadeSet, "fade_set", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "fade_active",
        JS_NewCFunction(context, jsFadeActive, "fade_active", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "fade_done",
        JS_NewCFunction(context, jsFadeDone, "fade_done", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "fade_alpha",
        JS_NewCFunction(context, jsFadeAlpha, "fade_alpha", 0)
    );

    JS_FreeValue(context, global);
}
