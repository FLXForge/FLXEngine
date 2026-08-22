#pragma once

#include "RuntimeObject.h"

class RuntimeHelpers
{
public:
    static void beginMechanicsFrame(RuntimeObject& object);

    static void moveHorizontal(RuntimeObject& object, float intent, float delta);
    static void moveVertical(RuntimeObject& object, float intent, float delta);

    static void advance(RuntimeObject& object, float delta);

    static void reflectX(RuntimeObject& object);
    static void reflectY(RuntimeObject& object);

    static void position(RuntimeObject& object, float x, float y);
    static void positionOrigin(RuntimeObject& object);
    static void applySpeed(RuntimeObject& object, float speed);
    static void restoreSpeed(RuntimeObject& object);

    static void accelerate(RuntimeObject& object, float intent, float delta);
    static void rotate(RuntimeObject& object, float intent, float delta);
    static void applyFreeMechanics(RuntimeObject& object, float delta);

    static bool intersects(
        const RuntimeObject& a,
        const RuntimeObject& b
    );

    static void moveY(RuntimeObject& object, float direction, float delta);

    static void followY(
        RuntimeObject& follower,
        const RuntimeObject& target,
        float delta
    );
};
