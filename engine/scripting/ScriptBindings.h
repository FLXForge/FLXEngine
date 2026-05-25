#pragma once

struct JSContext;
class ScriptEngine;

class ScriptBindings
{
public:
    static void registerAll(
        JSContext* context,
        ScriptEngine* scriptEngine
    );
};