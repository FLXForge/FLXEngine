#include "MotionBindings.h"
#include "BindingHelpers.h"
#include "../../runtime/RuntimeConstants.h"
#include "../../runtime/RuntimeHelpers.h"
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

    RuntimeObject* runtimeObjectFromArgument(
        JSContext* context,
        JSValueConst value
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr)
        {
            return nullptr;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, value, "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            JS_FreeValue(context, idValue);
            return nullptr;
        }

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return object;
    }

    void writeRuntimeObjectState(
        JSContext* context,
        JSValueConst value,
        const RuntimeObject& object
    )
    {
        JS_SetPropertyStr(context, value, "x", JS_NewFloat64(context, object.position.x));
        JS_SetPropertyStr(context, value, "y", JS_NewFloat64(context, object.position.y));
        JS_SetPropertyStr(context, value, "speed", JS_NewFloat64(context, object.speed));
        JS_SetPropertyStr(context, value, "angle", JS_NewFloat64(context, object.angle));
        JS_SetPropertyStr(context, value, "velocityX", JS_NewFloat64(context, object.velocity.x));
        JS_SetPropertyStr(context, value, "velocityY", JS_NewFloat64(context, object.velocity.y));
    }

    JSValue jsMoveHorizontal(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 2 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        double intent = 0.0;
        JS_ToFloat64(context, &intent, argv[1]);

        RuntimeHelpers::moveHorizontal(
            *object,
            static_cast<float>(intent),
            static_cast<float>(frameDelta(context))
        );

        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsMoveVertical(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 2 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        double intent = 0.0;
        JS_ToFloat64(context, &intent, argv[1]);

        RuntimeHelpers::moveVertical(
            *object,
            static_cast<float>(intent),
            static_cast<float>(frameDelta(context))
        );

        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsAdvance(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeHelpers::advance(
            *object,
            static_cast<float>(frameDelta(context))
        );

        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsReflectX(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeHelpers::reflectX(*object);
        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsReflectY(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeHelpers::reflectY(*object);
        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsAccelerate(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        double intent = 1.0;

        if (argc >= 2)
        {
            JS_ToFloat64(context, &intent, argv[1]);
        }

        RuntimeHelpers::accelerate(
            *object,
            static_cast<float>(intent),
            static_cast<float>(frameDelta(context))
        );

        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsRotate(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        double intent = 1.0;

        if (argc >= 2)
        {
            JS_ToFloat64(context, &intent, argv[1]);
        }

        RuntimeHelpers::rotate(
            *object,
            static_cast<float>(intent),
            static_cast<float>(frameDelta(context))
        );

        writeRuntimeObjectState(context, argv[0], *object);

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

    JSValue jsAttachActive(
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

    JSValue jsPositionOrigin(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeHelpers::positionOrigin(*object);
        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsPosition(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 3 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        double x = object->position.x;
        double y = object->position.y;

        JS_ToFloat64(context, &x, argv[1]);
        JS_ToFloat64(context, &y, argv[2]);

        RuntimeHelpers::position(
            *object,
            static_cast<float>(x),
            static_cast<float>(y)
        );

        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsApplySpeed(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 2 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        double speed =
            object->speed;

        JS_ToFloat64(context, &speed, argv[1]);

        RuntimeHelpers::applySpeed(
            *object,
            static_cast<float>(speed)
        );

        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }

    JSValue jsRestoreSpeed(
        JSContext* context,
        JSValueConst,
        int argc,
        JSValueConst* argv
    )
    {
        RuntimeObject* object =
            argc < 1 ? nullptr : runtimeObjectFromArgument(context, argv[0]);

        if (object == nullptr)
        {
            return JS_UNDEFINED;
        }

        RuntimeHelpers::restoreSpeed(*object);
        writeRuntimeObjectState(context, argv[0], *object);

        return JS_UNDEFINED;
    }
}

void MotionBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(context, global, "move_horizontal", JS_NewCFunction(context, jsMoveHorizontal, "move_horizontal", 2));
    JS_SetPropertyStr(context, global, "move_vertical", JS_NewCFunction(context, jsMoveVertical, "move_vertical", 2));
    JS_SetPropertyStr(context, global, "advance", JS_NewCFunction(context, jsAdvance, "advance", 1));
    JS_SetPropertyStr(context, global, "follow_x", JS_NewCFunction(context, jsFollowX, "follow_x", 2));
    JS_SetPropertyStr(context, global, "follow_y", JS_NewCFunction(context, jsFollowY, "follow_y", 2));
    JS_SetPropertyStr(context, global, "attach", JS_NewCFunction(context, jsAttach, "attach", 1));
    JS_SetPropertyStr(context, global, "detach", JS_NewCFunction(context, jsDetach, "detach", 1));
    JS_SetPropertyStr(context, global, "attach_active", JS_NewCFunction(context, jsAttachActive, "attach_active", 1));
    JS_SetPropertyStr(context, global, "carry", JS_NewCFunction(context, jsCarry, "carry", 2));
    JS_SetPropertyStr(context, global, "reflect_x", JS_NewCFunction(context, jsReflectX, "reflect_x", 1));
    JS_SetPropertyStr(context, global, "reflect_y", JS_NewCFunction(context, jsReflectY, "reflect_y", 1));
    JS_SetPropertyStr(context, global, "accelerate", JS_NewCFunction(context, jsAccelerate, "accelerate", 2));
    JS_SetPropertyStr(context, global, "rotate", JS_NewCFunction(context, jsRotate, "rotate", 2));
    JS_SetPropertyStr(context, global, "position", JS_NewCFunction(context, jsPosition, "position", 3));
    JS_SetPropertyStr(context, global, "position_origin", JS_NewCFunction(context, jsPositionOrigin, "position_origin", 1));
    JS_SetPropertyStr(context, global, "apply_speed", JS_NewCFunction(context, jsApplySpeed, "apply_speed", 2));
    JS_SetPropertyStr(context, global, "restore_speed", JS_NewCFunction(context, jsRestoreSpeed, "restore_speed", 1));

    JS_FreeValue(context, global);
}
