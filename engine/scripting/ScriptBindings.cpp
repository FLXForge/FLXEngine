#include "ScriptBindings.h"
#include "ScriptEngine.h"

#include "../runtime/RuntimeConstants.h"
#include "../runtime/RuntimeObject.h"
#include "../debug/Logger.h"

#include <quickjs.h>
#include <raylib.h>

#include <cmath>
#include <cstdlib>
#include <random>
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

static JSValue jsKill(
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

    JS_SetPropertyStr(
        context,
        self,
        "alive",
        JS_NewBool(context, false)
    );

    return JS_UNDEFINED;
}

static JSValue jsDelta(
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

static JSValue jsProbability(
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

    if (base <= 0)
    {
        return JS_NewBool(context, false);
    }

    if (chance <= 0)
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

static JSValue jsRandom(
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
        min + static_cast<double>(GetRandomValue(0, 1000000)) / 1000000.0 * (max - min);

    return JS_NewFloat64(
        context,
        randomValue
    );
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
    JSValue velocityXValue = JS_GetPropertyStr(context, self, "velocityX");
    JSValue velocityYValue = JS_GetPropertyStr(context, self, "velocityY");
    JSValue motionValue = JS_GetPropertyStr(context, self, "motion");
    JSValue accelerationValue = JS_GetPropertyStr(context, motionValue, "acceleration");
    JSValue inertiaValue = JS_GetPropertyStr(context, motionValue, "inertia");

    double x = 0.0;
    double y = 0.0;
    double speed = 0.0;
    double angle = 0.0;
    double velocityX = 0.0;
    double velocityY = 0.0;
    double acceleration = 0.0;
    double inertia = 1.0;

    JS_ToFloat64(context, &x, xValue);
    JS_ToFloat64(context, &y, yValue);
    JS_ToFloat64(context, &speed, speedValue);
    JS_ToFloat64(context, &angle, angleValue);
    JS_ToFloat64(context, &velocityX, velocityXValue);
    JS_ToFloat64(context, &velocityY, velocityYValue);
    JS_ToFloat64(context, &acceleration, accelerationValue);
    JS_ToFloat64(context, &inertia, inertiaValue);

    const double delta = GetFrameTime();

    if (acceleration > 0.0)
    {
        x += velocityX * delta;
        y += velocityY * delta;

        velocityX *= inertia;
        velocityY *= inertia;

        JS_SetPropertyStr(context, self, "velocityX", JS_NewFloat64(context, velocityX));
        JS_SetPropertyStr(context, self, "velocityY", JS_NewFloat64(context, velocityY));
    }
    else
    {
        const double radians =
            (angle - 90.0) * DEG2RAD;

        x += std::cos(radians) * speed * delta;
        y += std::sin(radians) * speed * delta;
    }

    JS_SetPropertyStr(context, self, "x", JS_NewFloat64(context, x));
    JS_SetPropertyStr(context, self, "y", JS_NewFloat64(context, y));

    JS_FreeValue(context, xValue);
    JS_FreeValue(context, yValue);
    JS_FreeValue(context, speedValue);
    JS_FreeValue(context, angleValue);
    JS_FreeValue(context, velocityXValue);
    JS_FreeValue(context, velocityYValue);
    JS_FreeValue(context, accelerationValue);
    JS_FreeValue(context, inertiaValue);
    JS_FreeValue(context, motionValue);

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

static JSValue jsAccelerate(
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

    if (argc >= 2)
    {
        double amount = 0.0;
        JS_ToFloat64(context, &amount, argv[1]);

        JSValue speedValue = JS_GetPropertyStr(context, self, "speed");

        double speed = 0.0;
        JS_ToFloat64(context, &speed, speedValue);

        speed += amount;

        JS_SetPropertyStr(context, self, "speed", JS_NewFloat64(context, speed));

        JS_FreeValue(context, speedValue);

        return JS_UNDEFINED;
    }

    JSValue motionValue = JS_GetPropertyStr(context, self, "motion");
    JSValue accelerationValue = JS_GetPropertyStr(context, motionValue, "acceleration");
    JSValue maxSpeedValue = JS_GetPropertyStr(context, motionValue, "maxSpeed");
    JSValue angleValue = JS_GetPropertyStr(context, self, "angle");
    JSValue velocityXValue = JS_GetPropertyStr(context, self, "velocityX");
    JSValue velocityYValue = JS_GetPropertyStr(context, self, "velocityY");

    double acceleration = 0.0;
    double maxSpeed = 0.0;
    double angle = 0.0;
    double velocityX = 0.0;
    double velocityY = 0.0;

    JS_ToFloat64(context, &acceleration, accelerationValue);
    JS_ToFloat64(context, &maxSpeed, maxSpeedValue);
    JS_ToFloat64(context, &angle, angleValue);
    JS_ToFloat64(context, &velocityX, velocityXValue);
    JS_ToFloat64(context, &velocityY, velocityYValue);

    if (acceleration > 0.0)
    {
        const double radians = (angle - 90.0) * DEG2RAD;
        const double delta = GetFrameTime();

        velocityX += std::cos(radians) * acceleration * delta;
        velocityY += std::sin(radians) * acceleration * delta;

        if (maxSpeed > 0.0)
        {
            const double currentSpeed =
                std::sqrt(
                    velocityX * velocityX +
                    velocityY * velocityY
                );

            if (currentSpeed > maxSpeed)
            {
                const double factor = maxSpeed / currentSpeed;

                velocityX *= factor;
                velocityY *= factor;
            }
        }

        JS_SetPropertyStr(context, self, "velocityX", JS_NewFloat64(context, velocityX));
        JS_SetPropertyStr(context, self, "velocityY", JS_NewFloat64(context, velocityY));
    }

    JS_FreeValue(context, motionValue);
    JS_FreeValue(context, accelerationValue);
    JS_FreeValue(context, maxSpeedValue);
    JS_FreeValue(context, angleValue);
    JS_FreeValue(context, velocityXValue);
    JS_FreeValue(context, velocityYValue);

    return JS_UNDEFINED;
}

static JSValue jsRotate(
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

    JSValue angleValue =
        JS_GetPropertyStr(context, self, "angle");

    JSValue motionValue =
        JS_GetPropertyStr(context, self, "motion");

    JSValue rotationSpeedValue =
        JS_GetPropertyStr(context, motionValue, "rotationSpeed");

    double angle = 0.0;
    double rotationSpeed = 0.0;

    JS_ToFloat64(context, &angle, angleValue);
    JS_ToFloat64(context, &rotationSpeed, rotationSpeedValue);

    const double delta =
        GetFrameTime();

    angle += direction * rotationSpeed * delta;

    JS_SetPropertyStr(
        context,
        self,
        "angle",
        JS_NewFloat64(context, angle)
    );

    JS_FreeValue(context, angleValue);
    JS_FreeValue(context, motionValue);
    JS_FreeValue(context, rotationSpeedValue);

    return JS_UNDEFINED;
}

static JSValue jsKeyDownGeneric(
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

    return JS_NewBool(
        context,
        IsKeyDown(key)
    );
}

static JSValue jsKeyPressedGeneric(
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

    return JS_NewBool(
        context,
        IsKeyPressed(key)
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

static JSValue jsSpawn(
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

    const char* spawnName =
        JS_ToCString(context, argv[1]);

    if (spawnName == nullptr)
    {
        return JS_UNDEFINED;
    }

    JSValue idValue =
        JS_GetPropertyStr(context, self, "id");

    const char* objectId =
        JS_ToCString(context, idValue);

    if (objectId == nullptr)
    {
        JS_FreeCString(context, spawnName);
        JS_FreeValue(context, idValue);
        return JS_UNDEFINED;
    }

    RuntimeObject* source =
        activeScriptEngine->findObjectByRuntimeId(objectId);

    if (source == nullptr)
    {
        Logger::warning(
            "spawn",
            "Spawner object not found: " + std::string(objectId)
        );

        JS_FreeCString(context, spawnName);
        JS_FreeCString(context, objectId);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    auto it =
        source->spawns.find(spawnName);

    if (it == source->spawns.end())
    {
        Logger::warning(
            "spawn",
            "Spawn not found: " + std::string(spawnName) +
            " in " + std::string(objectId)
        );

        JS_FreeCString(context, spawnName);
        JS_FreeCString(context, objectId);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    const SpawnDefinition& spawnDefinition =
        it->second;

    RuntimeObject* prefab =
        activeScriptEngine->findPrefabByName(spawnDefinition.prefab);

    if (prefab == nullptr)
    {
        Logger::warning(
            "spawn",
            "Prefab not found: " + spawnDefinition.prefab
        );

        JS_FreeCString(context, spawnName);
        JS_FreeCString(context, objectId);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    Logger::info(
        "spawn",
        "Prefab ready: " + prefab->name
    );

    activeScriptEngine->spawnObject(
        *source,
        spawnDefinition,
        *prefab
    );

    JS_FreeCString(context, spawnName);
    JS_FreeCString(context, objectId);
    JS_FreeValue(context, idValue);

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

    JS_SetPropertyStr(context, global, "KEY_UP", JS_NewInt32(context, KEY_UP));
    JS_SetPropertyStr(context, global, "KEY_DOWN", JS_NewInt32(context, KEY_DOWN));
    JS_SetPropertyStr(context, global, "KEY_LEFT", JS_NewInt32(context, KEY_LEFT));
    JS_SetPropertyStr(context, global, "KEY_RIGHT", JS_NewInt32(context, KEY_RIGHT));
    JS_SetPropertyStr(context, global, "KEY_SPACE", JS_NewInt32(context, KEY_SPACE));

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
        "rotate",
        JS_NewCFunction(context, jsRotate, "rotate", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "to_origin",
        JS_NewCFunction(context, jsToOrigin, "to_origin", 1)
    );

    JS_SetPropertyStr(
        context,
        global,
        "spawn",
        JS_NewCFunction(context, jsSpawn, "spawn", 2)
    );

    JSValue key = JS_NewObject(context);

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

    JS_SetPropertyStr(
        context,
        global,
        "Key",
        key
    );

    JS_FreeValue(context, global);
}