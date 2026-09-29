#pragma once

#include "../runtime/ObjectDefinition.h"

#include <raylib.h>

#include <string>
#include <vector>

class RuntimeObject;

struct EffectiveCollider
{
    const RuntimeObject* owner = nullptr;
    std::string name;
    std::string type = "box";
    std::vector<std::string> with;
    bool declaredEnabled = true;
    bool stateAvailable = true;
    bool effective = false;
    Vector2 center = Vector2{ 0.0f, 0.0f };
    Vector2 halfSize = Vector2{ 0.0f, 0.0f };
    float angle = 0.0f;
    Rectangle broadBounds = Rectangle{ 0.0f, 0.0f, 0.0f, 0.0f };
};

