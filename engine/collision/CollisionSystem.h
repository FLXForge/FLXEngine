#pragma once

#include <vector>

struct CollisionDebugFrame;
class RuntimeObject;
class ScriptEngine;

class CollisionSystem
{
public:
    static void run(
        std::vector<RuntimeObject>& objects,
        ScriptEngine& scriptEngine,
        CollisionDebugFrame* debugFrame = nullptr
    );
};
