#pragma once

#include "RuntimeObject.h"

class RuntimeHelpers
{
public:
    static void moveY(RuntimeObject& object, float direction, float delta);

    static void advance(RuntimeObject& object, float delta);

    static void bounceX(RuntimeObject& object);
    static void bounceY(RuntimeObject& object);

    static void toOrigin(RuntimeObject& object);

    static void accelerate(RuntimeObject& object, float amount);

    static bool intersects(
        const RuntimeObject& a,
        const RuntimeObject& b
    );

    static void followY(
        RuntimeObject& follower,
        const RuntimeObject& target,
        float delta
    );
};