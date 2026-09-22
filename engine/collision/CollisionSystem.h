#pragma once

#include <vector>

struct CollisionDebugFrame;
class RuntimeObject;
class ScriptEngine;

#ifdef FLX_TESTING
struct CollisionSystemStats
{
    size_t sourceBuilds = 0;
    size_t targetBuilds = 0;
    size_t candidateObjects = 0;
    size_t narrowPhaseCalls = 0;
    size_t callbackInvocations = 0;
};
#endif

class CollisionSystem
{
public:
    static void run(
        std::vector<RuntimeObject>& objects,
        ScriptEngine& scriptEngine,
        CollisionDebugFrame* debugFrame = nullptr
    );

#ifdef FLX_TESTING
    static void run(
        std::vector<RuntimeObject>& objects,
        ScriptEngine& scriptEngine,
        CollisionSystemStats& stats,
        CollisionDebugFrame* debugFrame = nullptr
    );
#endif
};
