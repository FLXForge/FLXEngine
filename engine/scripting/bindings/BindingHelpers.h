#pragma once

#include "../ScriptEngine.h"

#include <quickjs.h>

inline ScriptEngine* scriptEngineFromContext(JSContext* context)
{
    return static_cast<ScriptEngine*>(
        JS_GetContextOpaque(context)
    );
}
