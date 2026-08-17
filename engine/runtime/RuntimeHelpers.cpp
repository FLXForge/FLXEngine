#include "RuntimeHelpers.h"
#include "RuntimeConstants.h"

#include <algorithm>
#include <cmath>
#include <raylib.h>

namespace
{
    Rectangle getBox(
        const RuntimeObject& object
    )
    {
        return Rectangle{
            object.position.x,
            object.position.y,
            object.size.x,
            object.size.y
        };
    }

    Vector2 getCenter(
        const RuntimeObject& object
    )
    {
        if (object.shapeType == "block")
        {
            return Vector2{
                object.position.x + object.size.x / 2.0f,
                object.position.y + object.size.y / 2.0f
            };
        }

        return object.position;
    }

    float getRadius(
        const RuntimeObject& object
    )
    {
        if (object.collisionRadius > 0.0f)
        {
            return object.collisionRadius;
        }

        return std::max(
            object.size.x,
            object.size.y
        ) / 2.0f;
    }

    bool boxIntersects(
        const RuntimeObject& a,
        const RuntimeObject& b
    )
    {
        const Rectangle aBox =
            getBox(a);

        const Rectangle bBox =
            getBox(b);

        return CheckCollisionRecs(
            aBox,
            bBox
        );
    }

    bool circleIntersects(
        const RuntimeObject& a,
        const RuntimeObject& b
    )
    {
        const Vector2 aCenter =
            getCenter(a);

        const Vector2 bCenter =
            getCenter(b);

        const float dx =
            aCenter.x - bCenter.x;

        const float dy =
            aCenter.y - bCenter.y;

        const float radius =
            getRadius(a) + getRadius(b);

        return dx * dx + dy * dy <= radius * radius;
    }

    bool circleBoxIntersects(
        const RuntimeObject& circle,
        const RuntimeObject& box
    )
    {
        const Vector2 center =
            getCenter(circle);

        const Rectangle rect =
            getBox(box);

        const float closestX =
            std::max(
                rect.x,
                std::min(center.x, rect.x + rect.width)
            );

        const float closestY =
            std::max(
                rect.y,
                std::min(center.y, rect.y + rect.height)
            );

        const float dx =
            center.x - closestX;

        const float dy =
            center.y - closestY;

        const float radius =
            getRadius(circle);

        return dx * dx + dy * dy <= radius * radius;
    }
}

void RuntimeHelpers::moveY(
    RuntimeObject& object,
    float direction,
    float delta
)
{
    const float sign =
        direction < 0.0f
        ? -1.0f
        : direction > 0.0f
            ? 1.0f
            : 0.0f;

    object.position.y += sign * object.speed * delta;
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
    if (
        a.collisionType == "none" ||
        b.collisionType == "none"
        )
    {
        return false;
    }

    if (
        a.collisionType == "circle" &&
        b.collisionType == "circle"
        )
    {
        return circleIntersects(
            a,
            b
        );
    }

    if (
        a.collisionType == "circle" &&
        b.collisionType == "box"
        )
    {
        return circleBoxIntersects(
            a,
            b
        );
    }

    if (
        a.collisionType == "box" &&
        b.collisionType == "circle"
        )
    {
        return circleBoxIntersects(
            b,
            a
        );
    }

    return boxIntersects(
        a,
        b
    );
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
