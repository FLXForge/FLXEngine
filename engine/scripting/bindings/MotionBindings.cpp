#include "MotionBindings.h"
#include "BindingHelpers.h"
#include "../../runtime/RuntimeConstants.h"
#include "../../runtime/RuntimeObject.h"

#include <quickjs.h>
#include <raylib.h>

#include <algorithm>
#include <cmath>

namespace
{
    double frameDelta(JSContext* context)
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr)
        {
            return 0.0;
        }

        return scriptEngine->getFrameDelta();
    }

    JSValue jsMoveX(
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

        double direction = 0.0;
        JS_ToFloat64(context, &direction, argv[1]);

        JSValue xValue =
            JS_GetPropertyStr(context, argv[0], "x");

        JSValue speedValue =
            JS_GetPropertyStr(context, argv[0], "speed");

        double x = 0.0;
        double speed = 120.0;

        JS_ToFloat64(context, &x, xValue);
        JS_ToFloat64(context, &speed, speedValue);

        x += direction * speed * frameDelta(context);

        JS_SetPropertyStr(
            context,
            argv[0],
            "x",
            JS_NewFloat64(context, x)
        );

        JS_FreeValue(context, xValue);
        JS_FreeValue(context, speedValue);

        return JS_UNDEFINED;
    }

    JSValue jsMoveY(
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

        double direction = 0.0;
        JS_ToFloat64(context, &direction, argv[1]);

        JSValue yValue =
            JS_GetPropertyStr(context, argv[0], "y");

        JSValue speedValue =
            JS_GetPropertyStr(context, argv[0], "speed");

        double y = 0.0;
        double speed = 120.0;

        JS_ToFloat64(context, &y, yValue);
        JS_ToFloat64(context, &speed, speedValue);

        y += direction * speed * frameDelta(context);

        JS_SetPropertyStr(
            context,
            argv[0],
            "y",
            JS_NewFloat64(context, y)
        );

        JS_FreeValue(context, yValue);
        JS_FreeValue(context, speedValue);

        return JS_UNDEFINED;
    }

    JSValue jsAdvance(
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

        const double delta =
            frameDelta(context);

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

    JSValue jsBounceX(
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

        JSValue angleValue =
            JS_GetPropertyStr(context, argv[0], "angle");

        double angle = 0.0;
        JS_ToFloat64(context, &angle, angleValue);

        JS_SetPropertyStr(
            context,
            argv[0],
            "angle",
            JS_NewFloat64(context, -angle)
        );

        JS_FreeValue(context, angleValue);

        return JS_UNDEFINED;
    }

    JSValue jsBounceY(
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

        JSValue angleValue =
            JS_GetPropertyStr(context, argv[0], "angle");

        double angle = 0.0;
        JS_ToFloat64(context, &angle, angleValue);

        JS_SetPropertyStr(
            context,
            argv[0],
            "angle",
            JS_NewFloat64(context, 180.0 - angle)
        );

        JS_FreeValue(context, angleValue);

        return JS_UNDEFINED;
    }

    JSValue jsAccelerate(
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

            JSValue speedValue =
                JS_GetPropertyStr(context, self, "speed");

            double speed = 0.0;
            JS_ToFloat64(context, &speed, speedValue);

            JS_SetPropertyStr(
                context,
                self,
                "speed",
                JS_NewFloat64(context, speed + amount)
            );

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
            const double radians =
                (angle - 90.0) * DEG2RAD;

            const double delta =
                frameDelta(context);

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
                    const double factor =
                        maxSpeed / currentSpeed;

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

    JSValue jsRotate(
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

        angle += direction * rotationSpeed * frameDelta(context);

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

    JSValue jsFollowY(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 2 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        const char* targetName =
            JS_ToCString(context, argv[1]);

        if (targetName == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeObject* target =
            scriptEngine->findObjectByName(targetName);

        JS_FreeCString(context, targetName);

        if (target == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue yValue =
            JS_GetPropertyStr(context, argv[0], "y");

        JSValue heightValue =
            JS_GetPropertyStr(context, argv[0], "height");

        JSValue speedValue =
            JS_GetPropertyStr(context, argv[0], "speed");

        double y = 0.0;
        double height = 0.0;
        double speed = 120.0;

        JS_ToFloat64(context, &y, yValue);
        JS_ToFloat64(context, &height, heightValue);
        JS_ToFloat64(context, &speed, speedValue);

        const double distance =
            (target->position.y + target->size.y / 2.0) -
            (y + height / 2.0);

        const double maxStep =
            std::abs(speed) * frameDelta(context);

        if (std::abs(distance) <= maxStep)
        {
            y += distance;
        }
        else if (distance > 0.0)
        {
            y += maxStep;
        }
        else if (distance < 0.0)
        {
            y -= maxStep;
        }

        JS_SetPropertyStr(
            context,
            argv[0],
            "y",
            JS_NewFloat64(context, y)
        );

        JS_FreeValue(context, yValue);
        JS_FreeValue(context, heightValue);
        JS_FreeValue(context, speedValue);

        return JS_UNDEFINED;
    }

    JSValue jsFollowX(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 2 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        const char* targetName =
            JS_ToCString(context, argv[1]);

        if (targetName == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeObject* target =
            scriptEngine->findObjectByName(targetName);

        JS_FreeCString(context, targetName);

        if (target == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue xValue =
            JS_GetPropertyStr(context, argv[0], "x");

        JSValue widthValue =
            JS_GetPropertyStr(context, argv[0], "width");

        JSValue speedValue =
            JS_GetPropertyStr(context, argv[0], "speed");

        double x = 0.0;
        double width = 0.0;
        double speed = 120.0;

        JS_ToFloat64(context, &x, xValue);
        JS_ToFloat64(context, &width, widthValue);
        JS_ToFloat64(context, &speed, speedValue);

        const double distance =
            (target->position.x + target->size.x / 2.0) -
            (x + width / 2.0);

        const double maxStep =
            std::abs(speed) * frameDelta(context);

        if (std::abs(distance) <= maxStep)
        {
            x += distance;
        }
        else if (distance > 0.0)
        {
            x += maxStep;
        }
        else if (distance < 0.0)
        {
            x -= maxStep;
        }

        JS_SetPropertyStr(
            context,
            argv[0],
            "x",
            JS_NewFloat64(context, x)
        );

        JS_FreeValue(context, xValue);
        JS_FreeValue(context, widthValue);
        JS_FreeValue(context, speedValue);

        return JS_UNDEFINED;
    }

    JSValue jsAttach(
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
            "attached",
            JS_NewBool(context, true)
        );

        return JS_UNDEFINED;
    }

    JSValue jsDetach(
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
            "attached",
            JS_NewBool(context, false)
        );

        return JS_UNDEFINED;
    }

    JSValue jsIsAttached(
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

        JSValue attachedValue =
            JS_GetPropertyStr(context, argv[0], "attached");

        const bool attached =
            JS_ToBool(context, attachedValue);

        JS_FreeValue(context, attachedValue);

        return JS_NewBool(context, attached);
    }

    JSValue jsCarry(
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

        JSValue xValue =
            JS_GetPropertyStr(context, argv[0], "x");

        JSValue yValue =
            JS_GetPropertyStr(context, argv[0], "y");

        JSValue carrierXValue =
            JS_GetPropertyStr(context, argv[1], "x");

        JSValue carrierYValue =
            JS_GetPropertyStr(context, argv[1], "y");

        JSValue carrierPreviousXValue =
            JS_GetPropertyStr(context, argv[1], "previousX");

        JSValue carrierPreviousYValue =
            JS_GetPropertyStr(context, argv[1], "previousY");

        double x = 0.0;
        double y = 0.0;
        double carrierX = 0.0;
        double carrierY = 0.0;
        double carrierPreviousX = 0.0;
        double carrierPreviousY = 0.0;

        JS_ToFloat64(context, &x, xValue);
        JS_ToFloat64(context, &y, yValue);
        JS_ToFloat64(context, &carrierX, carrierXValue);
        JS_ToFloat64(context, &carrierY, carrierYValue);
        JS_ToFloat64(context, &carrierPreviousX, carrierPreviousXValue);
        JS_ToFloat64(context, &carrierPreviousY, carrierPreviousYValue);

        x += carrierX - carrierPreviousX;
        y += carrierY - carrierPreviousY;

        JS_SetPropertyStr(
            context,
            argv[0],
            "x",
            JS_NewFloat64(context, x)
        );

        JS_SetPropertyStr(
            context,
            argv[0],
            "y",
            JS_NewFloat64(context, y)
        );

        JS_FreeValue(context, xValue);
        JS_FreeValue(context, yValue);
        JS_FreeValue(context, carrierXValue);
        JS_FreeValue(context, carrierYValue);
        JS_FreeValue(context, carrierPreviousXValue);
        JS_FreeValue(context, carrierPreviousYValue);

        return JS_UNDEFINED;
    }

    JSValue jsToOrigin(
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

        JSValue originXValue = JS_GetPropertyStr(context, self, "originX");
        JSValue originYValue = JS_GetPropertyStr(context, self, "originY");
        JSValue originSpeedValue = JS_GetPropertyStr(context, self, "originSpeed");

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
}

void MotionBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(context, global, "move_x", JS_NewCFunction(context, jsMoveX, "move_x", 2));
    JS_SetPropertyStr(context, global, "move_y", JS_NewCFunction(context, jsMoveY, "move_y", 2));
    JS_SetPropertyStr(context, global, "advance", JS_NewCFunction(context, jsAdvance, "advance", 1));
    JS_SetPropertyStr(context, global, "follow_x", JS_NewCFunction(context, jsFollowX, "follow_x", 2));
    JS_SetPropertyStr(context, global, "follow_y", JS_NewCFunction(context, jsFollowY, "follow_y", 2));
    JS_SetPropertyStr(context, global, "attach", JS_NewCFunction(context, jsAttach, "attach", 1));
    JS_SetPropertyStr(context, global, "detach", JS_NewCFunction(context, jsDetach, "detach", 1));
    JS_SetPropertyStr(context, global, "is_attached", JS_NewCFunction(context, jsIsAttached, "is_attached", 1));
    JS_SetPropertyStr(context, global, "carry", JS_NewCFunction(context, jsCarry, "carry", 2));
    JS_SetPropertyStr(context, global, "bounce_x", JS_NewCFunction(context, jsBounceX, "bounce_x", 1));
    JS_SetPropertyStr(context, global, "bounce_y", JS_NewCFunction(context, jsBounceY, "bounce_y", 1));
    JS_SetPropertyStr(context, global, "accelerate", JS_NewCFunction(context, jsAccelerate, "accelerate", 2));
    JS_SetPropertyStr(context, global, "rotate", JS_NewCFunction(context, jsRotate, "rotate", 2));
    JS_SetPropertyStr(context, global, "to_origin", JS_NewCFunction(context, jsToOrigin, "to_origin", 1));

    JS_FreeValue(context, global);
}
