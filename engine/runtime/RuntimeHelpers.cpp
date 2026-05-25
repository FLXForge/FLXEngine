#include "RuntimeHelpers.h"
#include "RuntimeConstants.h"

#include <cmath>
#include <raylib.h>

void RuntimeHelpers::moveY(
    RuntimeObject& object,
    float direction,
    float delta
)
{
    object.position.y += direction * object.speed * delta;
}

void RuntimeHelpers::advance(
    RuntimeObject& object,
    float delta
)
{
    const float radians = object.angle * DEG2RAD;

    object.position.x += std::cos(radians) * object.speed * delta;
    object.position.y += std::sin(radians) * object.speed * delta;
}

void RuntimeHelpers::accelerate(RuntimeObject& object, float amount)
{
    object.speed += amount;
}

void RuntimeHelpers::bounceX(RuntimeObject& object)
{
    object.angle = 180.0f - object.angle;
}

void RuntimeHelpers::bounceY(RuntimeObject& object)
{
    object.angle = -object.angle;
}

void RuntimeHelpers::toOrigin(RuntimeObject& object)
{
    object.position = object.origin;
    object.speed = object.originSpeed;
}

bool RuntimeHelpers::intersects(
    const RuntimeObject& a,
    const RuntimeObject& b
)
{
    return
        a.position.x < b.position.x + b.size.x &&
        a.position.x + a.size.x > b.position.x &&
        a.position.y < b.position.y + b.size.y &&
        a.position.y + a.size.y > b.position.y;
}

void RuntimeHelpers::followY(
    RuntimeObject& follower,
    const RuntimeObject& target,
    float delta
)
{
    const float followerCenterY =
        follower.position.y + follower.size.y / 2.0f;

    const float targetCenterY =
        target.position.y + target.size.y / 2.0f;

    const float tolerance = 2.0f;

    if (followerCenterY < targetCenterY - tolerance)
    {
        moveY(follower, DOWN, delta);
    }
    else if (followerCenterY > targetCenterY + tolerance)
    {
        moveY(follower, UP, delta);
    }
}