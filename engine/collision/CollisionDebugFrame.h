#pragma once

#include "CollisionContact.h"
#include "EffectiveCollider.h"

#include <raylib.h>

#include <string>
#include <vector>

struct CollisionDebugContact
{
    std::string sourceId;
    std::string targetId;
    CollisionContact contact;
};

struct CollisionDebugRay
{
    Vector2 origin = Vector2{ 0.0f, 0.0f };
    Vector2 end = Vector2{ 0.0f, 0.0f };
    bool hit = false;
    Vector2 hitPoint = Vector2{ 0.0f, 0.0f };
    Vector2 hitNormal = Vector2{ 0.0f, 0.0f };
    std::string targetId;
    std::string collider;
};

struct CollisionDebugFrame
{
    std::vector<EffectiveCollider> colliders;
    std::vector<CollisionDebugContact> contacts;
    std::vector<CollisionDebugRay> rays;

    void clear()
    {
        colliders.clear();
        contacts.clear();
        rays.clear();
    }
};
