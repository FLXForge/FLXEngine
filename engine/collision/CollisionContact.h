#pragma once

#include <raylib.h>

#include <string>

struct CollisionContact
{
    std::string collider;
    std::string otherCollider;
    Vector2 normal = Vector2{ 0.0f, 0.0f };
    Vector2 point = Vector2{ 0.0f, 0.0f };
    float penetration = 0.0f;
};
