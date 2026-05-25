#pragma once

#include <quickjs.h>

struct ScriptModule
{
    JSValue start = JS_UNDEFINED;
    JSValue action = JS_UNDEFINED;
    JSValue motion = JS_UNDEFINED;
    JSValue collision = JS_UNDEFINED;
    JSValue draw = JS_UNDEFINED;
};