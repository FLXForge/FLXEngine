#pragma once

#include "CollisionContact.h"
#include "EffectiveCollider.h"

struct RayCollisionHit
{
    bool hit = false;
    float distance = 0.0f;
    Vector2 point = Vector2{ 0.0f, 0.0f };
    Vector2 normal = Vector2{ 0.0f, 0.0f };
};

class CollisionGeometry
{
public:
    static bool contact(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        CollisionContact& result
    );

    static RayCollisionHit ray(
        const EffectiveCollider& target,
        Vector2 origin,
        Vector2 direction,
        float maxDistance
    );
};
