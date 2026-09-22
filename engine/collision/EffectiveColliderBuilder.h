#pragma once

#include "EffectiveCollider.h"

#include <vector>

class RuntimeObject;
class ScriptEngine;

class EffectiveColliderBuilder
{
public:
    static std::vector<EffectiveCollider> build(
        const RuntimeObject& object,
        ScriptEngine& scriptEngine
    );
};
