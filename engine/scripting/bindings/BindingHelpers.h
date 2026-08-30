#pragma once

#include "../ScriptEngine.h"

#include <quickjs.h>

#include <string>

inline ScriptEngine* scriptEngineFromContext(JSContext* context)
{
    return static_cast<ScriptEngine*>(
        JS_GetContextOpaque(context)
    );
}

inline RuntimeObject* runtimeObjectViewFromArgument(
    JSContext* context,
    JSValueConst value
)
{
    if (!JS_IsObject(value))
    {
        return nullptr;
    }

    JSValue jsId =
        JS_GetPropertyStr(context, value, "id");

    const char* idText =
        JS_ToCString(context, jsId);

    std::string id =
        idText == nullptr ? "" : idText;

    if (idText != nullptr)
    {
        JS_FreeCString(context, idText);
    }

    JS_FreeValue(context, jsId);

    ScriptEngine* scriptEngine =
        scriptEngineFromContext(context);

    return scriptEngine == nullptr || id.empty()
        ? nullptr
        : scriptEngine->findObjectByRuntimeId(id);
}

inline void defineRuntimeViewValue(
    JSContext* context,
    JSValueConst object,
    const char* name,
    JSValue value
)
{
    JS_DefinePropertyValueStr(
        context,
        object,
        name,
        value,
        JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE
    );
}

inline void refreshRuntimeObjectView(
    JSContext* context,
    JSValueConst value,
    const RuntimeObject& object
)
{
    defineRuntimeViewValue(context, value, "id", JS_NewString(context, object.runtimeId.c_str()));
    defineRuntimeViewValue(context, value, "name", JS_NewString(context, object.name.c_str()));
    defineRuntimeViewValue(context, value, "group", JS_NewString(context, object.group.c_str()));
    defineRuntimeViewValue(context, value, "role", JS_NewString(context, object.role.c_str()));
    defineRuntimeViewValue(context, value, "alive", JS_NewBool(context, object.alive));
    defineRuntimeViewValue(context, value, "visible", JS_NewBool(context, object.visible));
    defineRuntimeViewValue(context, value, "x", JS_NewFloat64(context, object.position.x));
    defineRuntimeViewValue(context, value, "y", JS_NewFloat64(context, object.position.y));
    defineRuntimeViewValue(context, value, "width", JS_NewFloat64(context, object.size.x));
    defineRuntimeViewValue(context, value, "height", JS_NewFloat64(context, object.size.y));
    defineRuntimeViewValue(context, value, "speed", JS_NewFloat64(context, object.speed));
    defineRuntimeViewValue(context, value, "angle", JS_NewFloat64(context, object.angle));
    defineRuntimeViewValue(context, value, "velocityX", JS_NewFloat64(context, object.velocity.x));
    defineRuntimeViewValue(context, value, "velocityY", JS_NewFloat64(context, object.velocity.y));
    defineRuntimeViewValue(context, value, "rotationSpeed", JS_NewFloat64(context, object.rotationSpeed));
}
