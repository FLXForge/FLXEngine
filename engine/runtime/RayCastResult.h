#pragma once

#include <raylib.h>
#include <string>

struct RayCastResult
{
    bool hit = false;
    std::string objectId;
    std::string group;
    std::string collider;
    float distance = 0.0f;
    Vector2 point = Vector2{ 0.0f, 0.0f };
    Vector2 normal = Vector2{ 0.0f, 0.0f };
};
