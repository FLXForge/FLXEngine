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

inline void refreshRuntimeObjectView(
    JSContext* context,
    JSValueConst value,
    const RuntimeObject& object
)
{
    // RuntimeObject views expose live readonly getters, so mutations do not
    // need to copy values back into the JavaScript object.
    (void)context;
    (void)value;
    (void)object;
}
