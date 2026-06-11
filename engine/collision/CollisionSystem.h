#pragma once

#include <vector>

class RuntimeObject;
class ScriptEngine;

class CollisionSystem
{
public:
    static void run(
        std::vector<RuntimeObject>& objects,
        ScriptEngine& scriptEngine
    );
};
