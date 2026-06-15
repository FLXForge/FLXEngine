#include "CoreBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../runtime/RuntimeConstants.h"
#include "../../runtime/RayCastResult.h"
#include "../ScriptEngine.h"

#include <quickjs.h>
#include <raylib.h>

#include <string>

namespace
{
    JSValue createRayResult(
        JSContext* context,
        const RayCastResult& result
    )
    {
        JSValue object =
            JS_NewObject(context);

        JS_SetPropertyStr(
            context,
            object,
            "hit",
            JS_NewBool(context, result.hit)
        );

        JS_SetPropertyStr(
            context,
            object,
            "group",
            JS_NewString(context, result.group.c_str())
        );

        if (result.hit)
        {
            JS_SetPropertyStr(
                context,
                object,
                "distance",
                JS_NewFloat64(context, result.distance)
            );

            JS_SetPropertyStr(
                context,
                object,
                "x",
                JS_NewFloat64(context, result.point.x)
            );

            JS_SetPropertyStr(
                context,
                object,
                "y",
                JS_NewFloat64(context, result.point.y)
            );
        }

        return object;
    }

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

    JSValue jsKeepOnly(
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
            return JS_UNDEFINED;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            Logger::warning(
                "runtime",
                "keep_only called without a valid object"
            );

            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->keepOnly(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

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

    JSValue jsRay(
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
            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            JS_FreeValue(context, idValue);

            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        RuntimeObject* source =
            scriptEngine->findObjectByRuntimeId(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        if (source == nullptr)
        {
            return createRayResult(
                context,
                RayCastResult{}
            );
        }

        RuntimeObject querySource =
            *source;

        JSValue xValue =
            JS_GetPropertyStr(context, argv[0], "x");

        JSValue yValue =
            JS_GetPropertyStr(context, argv[0], "y");

        double x =
            querySource.position.x;

        double y =
            querySource.position.y;

        JS_ToFloat64(context, &x, xValue);
        JS_ToFloat64(context, &y, yValue);

        querySource.position = Vector2{
            static_cast<float>(x),
            static_cast<float>(y)
        };

        JS_FreeValue(context, xValue);
        JS_FreeValue(context, yValue);

        double angle = 0.0;
        double distance = 0.0;

        JS_ToFloat64(context, &angle, argv[1]);
        JS_ToFloat64(context, &distance, argv[2]);

        return createRayResult(
            context,
            scriptEngine->rayCast(
                querySource,
                static_cast<float>(angle),
                static_cast<float>(distance)
            )
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
        "keep_only",
        JS_NewCFunction(context, jsKeepOnly, "keep_only", 1)
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

    JS_SetPropertyStr(
        context,
        global,
        "ray",
        JS_NewCFunction(context, jsRay, "ray", 3)
    );

    JS_FreeValue(context, global);
}
