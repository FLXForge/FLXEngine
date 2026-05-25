#include "ScriptBindings.h"
#include "ScriptEngine.h"

#include "../runtime/RuntimeConstants.h"
#include "../runtime/RuntimeObject.h"
#include "../debug/Logger.h"

#include <quickjs.h>
#include <raylib.h>

#include <cmath>
#include <iostream>

static ScriptEngine* activeScriptEngine = nullptr;

static JSValue consoleLog(
    JSContext* context,
    JSValueConst thisValue,
    int argc,
    JSValueConst* argv
)
{
    for (int i = 0; i < argc; ++i)
    {
        const char* str =
            JS_ToCString(context, argv[i]);

        if (str)
        {
            Logger::debug(str);
            JS_FreeCString(context, str);
        }

        if (i < argc - 1)
        {
            Logger::debug(" ");
        }
    }

    return JS_UNDEFINED;
}

static JSValue jsMoveX(
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

    JSValue self = argv[0];

    double direction = 0.0;
    JS_ToFloat64(context, &direction, argv[1]);

    JSValue xValue =
        JS_GetPropertyStr(context, self, "x");

    JSValue speedValue =
        JS_GetPropertyStr(context, self, "speed");

    double x = 0.0;
    double speed = 120.0;

    JS_ToFloat64(context, &x, xValue);
    JS_ToFloat64(context, &speed, speedValue);

    const double delta = GetFrameTime();

    x += direction * speed * delta;

    JS_SetPropertyStr(
        context,
        self,
        "x",
        JS_NewFloat64(context, x)
    );

    JS_FreeValue(context, xValue);
    JS_FreeValue(context, speedValue);

    return JS_UNDEFINED;
}

static JSValue jsAdvance(
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

    JSValue self = argv[0];

    JSValue xValue = JS_GetPropertyStr(context, self, "x");
    JSValue yValue = JS_GetPropertyStr(context, self, "y");
    JSValue speedValue = JS_GetPropertyStr(context, self, "speed");
    JSValue angleValue = JS_GetPropertyStr(context, self, "angle");

    double x = 0.0;
    double y = 0.0;
    double speed = 0.0;
    double angle = 0.0;

    JS_ToFloat64(context, &x, xValue);
    JS_ToFloat64(context, &y, yValue);
    JS_ToFloat64(context, &speed, speedValue);
    JS_ToFloat64(context, &angle, angleValue);

    const double radians = angle * DEG2RAD;
    const double delta = GetFrameTime();

    x += std::cos(radians) * speed * delta;
    y += std::sin(radians) * speed * delta;

    JS_SetPropertyStr(context, self, "x", JS_NewFloat64(context, x));
    JS_SetPropertyStr(context, self, "y", JS_NewFloat64(context, y));

    JS_FreeValue(context, xValue);
    JS_FreeValue(context, yValue);
    JS_FreeValue(context, speedValue);
    JS_FreeValue(context, angleValue);

    return JS_UNDEFINED;
}

static JSValue jsMoveY(
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

    JSValue self = argv[0];

    double direction = 0.0;
    JS_ToFloat64(context, &direction, argv[1]);

    JSValue yValue =
        JS_GetPropertyStr(context, self, "y");

    JSValue speedValue =
        JS_GetPropertyStr(context, self, "speed");

    double y = 0.0;
    double speed = 120.0;

    JS_ToFloat64(context, &y, yValue);
    JS_ToFloat64(context, &speed, speedValue);

    const double delta = GetFrameTime();

    y += direction * speed * delta;

    JS_SetPropertyStr(
        context,
        self,
        "y",
        JS_NewFloat64(context, y)
    );

    JS_FreeValue(context, yValue);
    JS_FreeValue(context, speedValue);

    return JS_UNDEFINED;
}

static JSValue jsBounceX(
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

    JSValue self = argv[0];

    JSValue angleValue =
        JS_GetPropertyStr(context, self, "angle");

    double angle = 0.0;
    JS_ToFloat64(context, &angle, angleValue);

    angle = 180.0 - angle;

    JS_SetPropertyStr(
        context,
        self,
        "angle",
        JS_NewFloat64(context, angle)
    );

    JS_FreeValue(context, angleValue);

    return JS_UNDEFINED;
}

static JSValue jsBounceY(
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

    JSValue self = argv[0];

    JSValue angleValue =
        JS_GetPropertyStr(context, self, "angle");

    double angle = 0.0;
    JS_ToFloat64(context, &angle, angleValue);

    angle = -angle;

    JS_SetPropertyStr(
        context,
        self,
        "angle",
        JS_NewFloat64(context, angle)
    );

    JS_FreeValue(context, angleValue);

    return JS_UNDEFINED;
}

static JSValue jsAccelerate(
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

    JSValue self = argv[0];

    double amount = 0.0;
    JS_ToFloat64(context, &amount, argv[1]);

    JSValue speedValue =
        JS_GetPropertyStr(context, self, "speed");

    double speed = 0.0;
    JS_ToFloat64(context, &speed, speedValue);

    speed += amount;

    JS_SetPropertyStr(
        context,
        self,
        "speed",
        JS_NewFloat64(context, speed)
    );

    JS_FreeValue(context, speedValue);

    return JS_UNDEFINED;
}



static JSValue jsKeyUp(
    JSContext* context,
    JSValueConst thisValue,
    int argc,
    JSValueConst* argv
)
{
    return JS_NewBool(
        context,
        IsKeyDown(KEY_UP)
    );
}

static JSValue jsKeyDown(
    JSContext* context,
    JSValueConst thisValue,
    int argc,
    JSValueConst* argv
)
{
    return JS_NewBool(
        context,
        IsKeyDown(KEY_DOWN)
    );
}

static JSValue jsFollowY(
    JSContext* context,
    JSValueConst thisValue,
    int argc,
    JSValueConst* argv
)
{
    if (argc < 2 || activeScriptEngine == nullptr)
    {
        return JS_UNDEFINED;
    }

    JSValue self = argv[0];

    const char* targetName =
        JS_ToCString(context, argv[1]);

    if (targetName == nullptr)
    {
        return JS_UNDEFINED;
    }

    RuntimeObject* target =
        activeScriptEngine->findObjectByName(targetName);

    JS_FreeCString(context, targetName);

    if (target == nullptr)
    {
        return JS_UNDEFINED;
    }

    JSValue yValue =
        JS_GetPropertyStr(context, self, "y");

    JSValue heightValue =
        JS_GetPropertyStr(context, self, "height");

    JSValue speedValue =
        JS_GetPropertyStr(context, self, "speed");

    double y = 0.0;
    double height = 0.0;
    double speed = 120.0;

    JS_ToFloat64(context, &y, yValue);
    JS_ToFloat64(context, &height, heightValue);
    JS_ToFloat64(context, &speed, speedValue);

    const double followerCenterY =
        y + height / 2.0;

    const double targetCenterY =
        target->position.y + target->size.y / 2.0;

    const double tolerance = 2.0;
    const double delta = GetFrameTime();

    if (followerCenterY < targetCenterY - tolerance)
    {
        y += DOWN * speed * delta;
    }
    else if (followerCenterY > targetCenterY + tolerance)
    {
        y += UP * speed * delta;
    }

    JS_SetPropertyStr(
        context,
        self,
        "y",
        JS_NewFloat64(context, y)
    );

    JS_FreeValue(context, yValue);
    JS_FreeValue(context, heightValue);
    JS_FreeValue(context, speedValue);

    return JS_UNDEFINED;
}

static JSValue jsToOrigin(
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

    JSValue self = argv[0];

    JSValue originXValue =
        JS_GetPropertyStr(context, self, "originX");

    JSValue originYValue =
        JS_GetPropertyStr(context, self, "originY");

    JSValue originSpeedValue =
        JS_GetPropertyStr(context, self, "originSpeed");

    double originX = 0.0;
    double originY = 0.0;
    double originSpeed = 0.0;

    JS_ToFloat64(context, &originX, originXValue);
    JS_ToFloat64(context, &originY, originYValue);
    JS_ToFloat64(context, &originSpeed, originSpeedValue);

    JS_SetPropertyStr(context, self, "x", JS_NewFloat64(context, originX));
    JS_SetPropertyStr(context, self, "y", JS_NewFloat64(context, originY));
    JS_SetPropertyStr(context, self, "speed", JS_NewFloat64(context, originSpeed));

    JS_FreeValue(context, originXValue);
    JS_FreeValue(context, originYValue);
    JS_FreeValue(context, originSpeedValue);

    return JS_UNDEFINED;
}


void ScriptBindings::registerAll(
    JSContext* context,
    ScriptEngine* scriptEngine
)
{
    activeScriptEngine = scriptEngine;

    JSValue global = JS_GetGlobalObject(context);

    JSValue console = JS_NewObject(context);

    JS_SetPropertyStr(
        context,
        console,
        "log",
        JS_NewCFunction(context, consoleLog, "log", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "console",
        console
    );

    JS_SetPropertyStr(context, global, "UP", JS_NewInt32(context, UP));
    JS_SetPropertyStr(context, global, "DOWN", JS_NewInt32(context, DOWN));
    JS_SetPropertyStr(context, global, "LEFT", JS_NewInt32(context, LEFT));
    JS_SetPropertyStr(context, global, "RIGHT", JS_NewInt32(context, RIGHT));
    JS_SetPropertyStr(context, global, "STOP", JS_NewInt32(context, STOP));

    JS_SetPropertyStr(
        context,
        global,
        "move_x",
        JS_NewCFunction(context, jsMoveX, "move_x", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "move_y",
        JS_NewCFunction(context, jsMoveY, "move_y", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "advance",
        JS_NewCFunction(context, jsAdvance, "advance", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "follow_y",
        JS_NewCFunction(context, jsFollowY, "follow_y", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "bounce_x",
        JS_NewCFunction(context, jsBounceX, "bounce_x", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "bounce_y",
        JS_NewCFunction(context, jsBounceY, "bounce_y", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "accelerate",
        JS_NewCFunction(context, jsAccelerate, "accelerate", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "to_origin",
        JS_NewCFunction(context, jsToOrigin, "to_origin", 1)
    );

    JSValue key = JS_NewObject(context);

    JS_SetPropertyStr(
        context,
        key,
        "up",
        JS_NewCFunction(context, jsKeyUp, "up", 0)
    );

    JS_SetPropertyStr(
        context,
        key,
        "down",
        JS_NewCFunction(context, jsKeyDown, "down", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "Key",
        key
    );

    JS_FreeValue(context, global);
}