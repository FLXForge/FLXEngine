#include "CoreBindings.h"
#include "../../debug/Logger.h"
#include "../../runtime/RuntimeConstants.h"

#include <quickjs.h>
#include <raylib.h>

#include <string>

namespace
{
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
        if (argc < 1)
        {
            return JS_UNDEFINED;
        }

        JS_SetPropertyStr(
            context,
            argv[0],
            "alive",
            JS_NewBool(context, false)
        );

        return JS_UNDEFINED;
    }

    JSValue jsDelta(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        return JS_NewFloat64(
            context,
            GetFrameTime()
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

    JS_FreeValue(context, global);
}
