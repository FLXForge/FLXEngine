#include "DrawBindings.h"
#include "BindingHelpers.h"

#include <quickjs.h>

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

        const bool local =
            isRuntimeObjectView(context, argv[0]);

        const int xIndex =
            local ? 1 : 0;

        const int yIndex =
            local ? 2 : 1;

        const int textIndex =
            local ? 3 : 2;

        if (argc <= textIndex)
        {
            return JS_UNDEFINED;
        }

        int fontSize = 10;

        const int fontSizeIndex =
            textIndex + 1;

        if (argc > fontSizeIndex)
        {
            JS_ToInt32(context, &fontSize, argv[fontSizeIndex]);
        }

        const std::string colorText =
            optionalString(context, argc, argv, fontSizeIndex + 1, "white");

        const Color color =
            scriptEngine->parseColor(colorText, WHITE);

        double x = 0.0;
        double y = 0.0;

        JS_ToFloat64(context, &x, argv[xIndex]);
        JS_ToFloat64(context, &y, argv[yIndex]);

        const char* text =
            JS_ToCString(context, argv[textIndex]);

        if (text == nullptr)
        {
            return JS_UNDEFINED;
        }

        if (local)
        {
            RuntimeObject* reference =
                runtimeObjectViewFromArgument(context, argv[0]);

            if (reference != nullptr)
            {
                scriptEngine->drawLocalText(
                    *reference,
                    Vector2{ static_cast<float>(x), static_cast<float>(y) },
                    text,
                    fontSize,
                    color
                );
            }
        }
        else
        {
            scriptEngine->drawWorldText(
                Vector2{ static_cast<float>(x), static_cast<float>(y) },
                text,
                fontSize,
                color
            );
        }

        JS_FreeCString(context, text);

        return JS_UNDEFINED;
    }

    JSValue jsDrawPixel(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 2)
        {
            return JS_UNDEFINED;
        }

        const bool local =
            isRuntimeObjectView(context, argv[0]);

        const int xIndex =
            local ? 1 : 0;

        const int yIndex =
            local ? 2 : 1;

        if (argc <= yIndex)
        {
            return JS_UNDEFINED;
        }

        double x = 0.0;
        double y = 0.0;

        JS_ToFloat64(context, &x, argv[xIndex]);
        JS_ToFloat64(context, &y, argv[yIndex]);

        const std::string colorText =
            optionalString(context, argc, argv, yIndex + 1, "white");

        const Color color =
            scriptEngine->parseColor(colorText, WHITE);

        if (local)
        {
            RuntimeObject* reference =
                runtimeObjectViewFromArgument(context, argv[0]);

            if (reference != nullptr)
            {
                scriptEngine->drawLocalPixel(
                    *reference,
                    Vector2{ static_cast<float>(x), static_cast<float>(y) },
                    color
                );
            }
        }
        else
        {
            scriptEngine->drawWorldPixel(
                Vector2{ static_cast<float>(x), static_cast<float>(y) },
                color
            );
        }

        return JS_UNDEFINED;
    }

    JSValue jsDrawLine(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 4)
        {
            return JS_UNDEFINED;
        }

        const bool local =
            isRuntimeObjectView(context, argv[0]);

        const int xIndex =
            local ? 1 : 0;

        const int yIndex =
            local ? 2 : 1;

        const int x1Index =
            local ? 3 : 2;

        const int y1Index =
            local ? 4 : 3;

        if (argc <= y1Index)
        {
            return JS_UNDEFINED;
        }

        double x = 0.0;
        double y = 0.0;
        double x1 = 0.0;
        double y1 = 0.0;

        JS_ToFloat64(context, &x, argv[xIndex]);
        JS_ToFloat64(context, &y, argv[yIndex]);
        JS_ToFloat64(context, &x1, argv[x1Index]);
        JS_ToFloat64(context, &y1, argv[y1Index]);

        const std::string colorText =
            optionalString(context, argc, argv, y1Index + 1, "white");

        const Color color =
            scriptEngine->parseColor(colorText, WHITE);

        if (local)
        {
            RuntimeObject* reference =
                runtimeObjectViewFromArgument(context, argv[0]);

            if (reference != nullptr)
            {
                scriptEngine->drawLocalLine(
                    *reference,
                    Vector2{ static_cast<float>(x), static_cast<float>(y) },
                    Vector2{ static_cast<float>(x1), static_cast<float>(y1) },
                    color
                );
            }
        }
        else
        {
            scriptEngine->drawWorldLine(
                Vector2{ static_cast<float>(x), static_cast<float>(y) },
                Vector2{ static_cast<float>(x1), static_cast<float>(y1) },
                color
            );
        }

        return JS_UNDEFINED;
    }

    JSValue jsDrawRectangle(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr || argc < 4)
        {
            return JS_UNDEFINED;
        }

        const bool local =
            isRuntimeObjectView(context, argv[0]);

        const int xIndex =
            local ? 1 : 0;

        const int yIndex =
            local ? 2 : 1;

        const int widthIndex =
            local ? 3 : 2;

        const int heightIndex =
            local ? 4 : 3;

        if (argc <= heightIndex)
        {
            return JS_UNDEFINED;
        }

        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;

        JS_ToFloat64(context, &x, argv[xIndex]);
        JS_ToFloat64(context, &y, argv[yIndex]);
        JS_ToFloat64(context, &width, argv[widthIndex]);
        JS_ToFloat64(context, &height, argv[heightIndex]);

        const std::string colorText =
            optionalString(context, argc, argv, heightIndex + 1, "white");

        const Color color =
            scriptEngine->parseColor(colorText, WHITE);

        if (local)
        {
            RuntimeObject* reference =
                runtimeObjectViewFromArgument(context, argv[0]);

            if (reference != nullptr)
            {
                scriptEngine->drawLocalRectangle(
                    *reference,
                    Vector2{ static_cast<float>(x), static_cast<float>(y) },
                    Vector2{ static_cast<float>(width), static_cast<float>(height) },
                    color
                );
            }
        }
        else
        {
            scriptEngine->drawWorldRectangle(
                Vector2{ static_cast<float>(x), static_cast<float>(y) },
                Vector2{ static_cast<float>(width), static_cast<float>(height) },
                color
            );
        }

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
        JS_NewCFunction(context, jsDrawText, "draw_text", 6)
    );

    JS_SetPropertyStr(
        context,
        global,
        "draw_pixel",
        JS_NewCFunction(context, jsDrawPixel, "draw_pixel", 4)
    );

    JS_SetPropertyStr(
        context,
        global,
        "draw_line",
        JS_NewCFunction(context, jsDrawLine, "draw_line", 6)
    );

    JS_SetPropertyStr(
        context,
        global,
        "draw_rectangle",
        JS_NewCFunction(context, jsDrawRectangle, "draw_rectangle", 6)
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
