#pragma once

#include "../ScriptEngine.h"

#include <quickjs.h>

#include <cstdint>
#include <string>

inline constexpr const char* FlxRuntimeViewInternalId =
    "__flxRuntimeId";

inline constexpr const char* FlxRuntimeViewInternalInvocation =
    "__flxInvocationId";

inline ScriptEngine* scriptEngineFromContext(JSContext* context)
{
    return static_cast<ScriptEngine*>(
        JS_GetContextOpaque(context)
    );
}

inline bool runtimeObjectViewMetadata(
    JSContext* context,
    JSValueConst value,
    std::string& runtimeId,
    uint64_t& invocationId
)
{
    if (!JS_IsObject(value))
    {
        return false;
    }

    JSValue idValue =
        JS_GetPropertyStr(context, value, FlxRuntimeViewInternalId);

    JSValue invocationValue =
        JS_GetPropertyStr(context, value, FlxRuntimeViewInternalInvocation);

    const char* idText =
        JS_ToCString(context, idValue);

    int64_t rawInvocationId = 0;

    const bool valid =
        idText != nullptr &&
        JS_ToInt64(context, &rawInvocationId, invocationValue) == 0 &&
        rawInvocationId > 0;

    if (valid)
    {
        runtimeId =
            idText;
        invocationId =
            static_cast<uint64_t>(rawInvocationId);
    }

    if (idText != nullptr)
    {
        JS_FreeCString(context, idText);
    }

    JS_FreeValue(context, idValue);
    JS_FreeValue(context, invocationValue);

    return valid && !runtimeId.empty();
}

inline bool isRuntimeObjectView(
    JSContext* context,
    JSValueConst value
)
{
    std::string runtimeId;
    uint64_t invocationId = 0;

    return runtimeObjectViewMetadata(
        context,
        value,
        runtimeId,
        invocationId
    );
}

inline RuntimeObject* runtimeObjectViewFromArgument(
    JSContext* context,
    JSValueConst value
)
{
    std::string runtimeId;
    uint64_t invocationId = 0;

    ScriptEngine* scriptEngine =
        scriptEngineFromContext(context);

    if (scriptEngine == nullptr ||
        !runtimeObjectViewMetadata(
            context,
            value,
            runtimeId,
            invocationId
        ))
    {
        return nullptr;
    }

    return scriptEngine == nullptr
        ? nullptr
        : scriptEngine->resolveRuntimeObjectReference(
            runtimeId,
            invocationId
        );
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
