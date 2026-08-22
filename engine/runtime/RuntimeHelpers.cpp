#include "RuntimeHelpers.h"
#include "RuntimeConstants.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <cmath>
#include <raylib.h>

namespace
{
    float clampIntent(float intent)
    {
        return std::clamp(intent, -1.0f, 1.0f);
    }

    float clampMagnitude(float value, float limit)
    {
        if (limit <= 0.0f)
        {
            return value;
        }

        return std::clamp(value, -limit, limit);
    }

    float quantize(float value, float step)
    {
        if (step <= 0.0f)
        {
            return value;
        }

        return std::round(value / step) * step;
    }

    Vector2 polarVector(float angle, float speed)
    {
        const float radians =
            (angle - 90.0f) * DEG2RAD;

        return Vector2{
            std::cos(radians) * speed,
            std::sin(radians) * speed
        };
    }

    float vectorLength(Vector2 value)
    {
        return std::sqrt(
            value.x * value.x +
            value.y * value.y
        );
    }

    Vector2 clampVector(Vector2 value, float limit)
    {
        if (limit <= 0.0f)
        {
            return value;
        }

        const float length =
            vectorLength(value);

        if (length <= limit || length <= 0.0f)
        {
            return value;
        }

        const float factor =
            limit / length;

        return Vector2{
            value.x * factor,
            value.y * factor
        };
    }

    Vector2 normalizeDiagonalVector(Vector2 value)
    {
        const float length =
            vectorLength(value);

        const float axisMagnitude =
            std::max(
                std::abs(value.x),
                std::abs(value.y)
            );

        if (length <= 0.0f ||
            axisMagnitude <= 0.0f ||
            length <= axisMagnitude)
        {
            return value;
        }

        const float factor =
            axisMagnitude / length;

        return Vector2{
            value.x * factor,
            value.y * factor
        };
    }

    float retention(float inertia, float delta)
    {
        if (inertia <= 0.0f)
        {
            return 0.0f;
        }

        if (inertia >= 1.0f)
        {
            return 1.0f;
        }

        return std::pow(inertia, delta);
    }

    void applyDirectMotionDelta(RuntimeObject& object, Vector2 delta)
    {
        object.position.x -=
            object.frameMotionDelta.x;

        object.position.y -=
            object.frameMotionDelta.y;

        object.position.x +=
            delta.x;

        object.position.y +=
            delta.y;

        object.frameMotionDelta =
            delta;
    }

    float axisVelocity(
        float currentVelocity,
        float intent,
        const MechanicsAxisDefinition& axis,
        float delta
    )
    {
        const float sign =
            clampIntent(intent);

        if (axis.acceleration > 0.0f)
        {
            if (std::abs(sign) <= 0.00001f)
            {
                return currentVelocity *
                    retention(axis.inertia, delta);
            }

            return currentVelocity +
                sign * axis.acceleration * delta;
        }

        return sign * axis.speed.start;
    }

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

void RuntimeHelpers::beginMechanicsFrame(RuntimeObject& object)
{
    object.motionCommanded = false;
    object.rotationCommanded = false;
    object.frameMotionDelta = Vector2{ 0.0f, 0.0f };
}

void RuntimeHelpers::moveHorizontal(
    RuntimeObject& object,
    float intent,
    float delta
)
{
    const MechanicsAxisDefinition& axis =
        object.mechanicsMotion.horizontal;

    float velocity =
        axisVelocity(
            object.velocity.x,
            intent,
            axis,
            delta
        );

    velocity =
        clampMagnitude(velocity, axis.speed.limit);

    object.velocity.x =
        velocity;

    if (object.mechanicsMotion.diagonal == MechanicsDiagonalMode::Vector)
    {
        Vector2 effectiveVelocity =
            normalizeDiagonalVector(
                clampVector(
                    object.velocity,
                    object.mechanicsMotion.speed.limit
                )
            );

        const Vector2 movement =
            Vector2{
                quantize(effectiveVelocity.x * delta, axis.step),
                quantize(effectiveVelocity.y * delta, object.mechanicsMotion.vertical.step)
            };

        applyDirectMotionDelta(
            object,
            movement
        );
    }
    else
    {
        object.position.x +=
            quantize(object.velocity.x * delta, axis.step);
    }

    object.motionCommanded =
        true;
}

void RuntimeHelpers::moveVertical(
    RuntimeObject& object,
    float intent,
    float delta
)
{
    const MechanicsAxisDefinition& axis =
        object.mechanicsMotion.vertical;

    float velocity =
        axisVelocity(
            object.velocity.y,
            intent,
            axis,
            delta
        );

    velocity =
        clampMagnitude(velocity, axis.speed.limit);

    object.velocity.y =
        velocity;

    if (object.mechanicsMotion.diagonal == MechanicsDiagonalMode::Vector)
    {
        Vector2 effectiveVelocity =
            normalizeDiagonalVector(
                clampVector(
                    object.velocity,
                    object.mechanicsMotion.speed.limit
                )
            );

        const Vector2 movement =
            Vector2{
                quantize(effectiveVelocity.x * delta, object.mechanicsMotion.horizontal.step),
                quantize(effectiveVelocity.y * delta, axis.step)
            };

        applyDirectMotionDelta(
            object,
            movement
        );
    }
    else
    {
        object.position.y +=
            quantize(object.velocity.y * delta, axis.step);
    }

    object.motionCommanded =
        true;
}

void RuntimeHelpers::advance(
    RuntimeObject& object,
    float delta
)
{
    if (object.mechanicsType == MechanicsType::Polar)
    {
        if (std::abs(object.speed) <= 0.00001f)
        {
            object.speed =
                object.originSpeed;
        }
    }

    const Vector2 polarMovement =
        object.mechanicsType == MechanicsType::Polar
            ? polarVector(object.angle, object.speed)
            : Vector2{ 0.0f, 0.0f };

    const Vector2 effectiveVelocity =
        Vector2{
            object.velocity.x + polarMovement.x,
            object.velocity.y + polarMovement.y
        };

    const Vector2 movement =
        Vector2{
            quantize(effectiveVelocity.x * delta, object.mechanicsMotion.step),
            quantize(effectiveVelocity.y * delta, object.mechanicsMotion.step)
        };

    object.position.x +=
        movement.x;

    object.position.y +=
        movement.y;

    object.motionCommanded =
        true;
}

void RuntimeHelpers::accelerate(
    RuntimeObject& object,
    float intent,
    float delta
)
{
    const float sign =
        clampIntent(intent);

    const float acceleration =
        object.mechanicsMotion.acceleration;

    if (acceleration > 0.0f)
    {
        const Vector2 direction =
            polarVector(
                object.angle,
                acceleration * sign * delta
            );

        object.velocity.x +=
            direction.x;

        object.velocity.y +=
            direction.y;

        object.velocity =
            clampVector(
                object.velocity,
                object.mechanicsMotion.speed.limit
            );

        object.speed =
            vectorLength(object.velocity);
    }
    else if (std::abs(object.speed) <= 0.00001f)
    {
        object.speed =
            object.originSpeed * sign;
    }

    if (acceleration <= 0.0f)
    {
        object.speed =
            clampMagnitude(
                object.speed,
                object.mechanicsMotion.speed.limit
            );

        object.velocity =
            polarVector(
                object.angle,
                object.speed
            );
    }

    object.position.x +=
        quantize(object.velocity.x * delta, object.mechanicsMotion.step);

    object.position.y +=
        quantize(object.velocity.y * delta, object.mechanicsMotion.step);

    object.motionCommanded =
        true;
}

void RuntimeHelpers::rotate(
    RuntimeObject& object,
    float intent,
    float delta
)
{
    const float sign =
        clampIntent(intent);

    if (object.mechanicsRotation.acceleration > 0.0f)
    {
        object.angularVelocity +=
            sign * object.mechanicsRotation.acceleration * delta;
    }
    else
    {
        const float rotationSpeed =
            std::abs(object.rotationSpeed) > 0.00001f
                ? object.rotationSpeed
                : object.mechanicsRotation.speed.start;

        object.angularVelocity =
            sign * rotationSpeed;
    }

    object.angularVelocity =
        clampMagnitude(
            object.angularVelocity,
            object.mechanicsRotation.speed.limit
        );

    object.angle +=
        quantize(
            object.angularVelocity * delta,
            object.mechanicsRotation.step
        );

    object.rotationCommanded =
        true;
}

void RuntimeHelpers::reflectX(RuntimeObject& object)
{
    object.angle = -object.angle;
    object.velocity.x =
        -object.velocity.x;
}

void RuntimeHelpers::reflectY(RuntimeObject& object)
{
    object.angle = 180.0f - object.angle;
    object.velocity.y =
        -object.velocity.y;
}

void RuntimeHelpers::position(
    RuntimeObject& object,
    float x,
    float y
)
{
    object.position = Vector2{
        x,
        y
    };
}

void RuntimeHelpers::positionOrigin(RuntimeObject& object)
{
    object.position = object.origin;
}

void RuntimeHelpers::applySpeed(
    RuntimeObject& object,
    float speed
)
{
    const float requestedSpeed =
        speed;

    speed =
        clampMagnitude(
            speed,
            object.mechanicsMotion.speed.limit
        );

    if (std::abs(speed - requestedSpeed) > 0.00001f)
    {
        Logger::warning(
            "mechanics",
            "apply_speed requested value exceeds mechanics.motion.speed.limit"
        );
    }

    object.speed =
        speed;

    if (object.mechanicsType == MechanicsType::Polar)
    {
        object.velocity =
            polarVector(
                object.angle,
                object.speed
            );

        return;
    }

    const float length =
        vectorLength(object.velocity);

    if (length > 0.0f)
    {
        const float factor =
            speed / length;

        object.velocity.x *=
            factor;

        object.velocity.y *=
            factor;
    }
}

void RuntimeHelpers::restoreSpeed(RuntimeObject& object)
{
    object.speed = object.originSpeed;

    if (object.mechanicsType == MechanicsType::Polar)
    {
        object.velocity =
            polarVector(
                object.angle,
                object.speed
            );
    }
}

void RuntimeHelpers::applyFreeMechanics(
    RuntimeObject& object,
    float delta
)
{
    if (!object.motionCommanded)
    {
        if (object.mechanicsType == MechanicsType::Direct)
        {
            if (object.mechanicsMotion.horizontal.inertia > 0.0f)
            {
                object.velocity.x *=
                    retention(object.mechanicsMotion.horizontal.inertia, delta);
            }

            if (object.mechanicsMotion.vertical.inertia > 0.0f)
            {
                object.velocity.y *=
                    retention(object.mechanicsMotion.vertical.inertia, delta);
            }

            object.velocity =
                clampVector(
                    object.velocity,
                    object.mechanicsMotion.speed.limit
                );

            if (object.mechanicsMotion.horizontal.inertia <= 0.0f &&
                object.mechanicsMotion.vertical.inertia <= 0.0f)
            {
                return;
            }
        }
        else
        {
            object.speed *=
                retention(object.mechanicsMotion.inertia, delta);

            object.velocity =
                polarVector(
                    object.angle,
                    object.speed
                );
        }

        object.position.x +=
            quantize(object.velocity.x * delta, object.mechanicsMotion.step);

        object.position.y +=
            quantize(object.velocity.y * delta, object.mechanicsMotion.step);
    }

    if (!object.rotationCommanded)
    {
        object.angularVelocity *=
            retention(object.mechanicsRotation.inertia, delta);

        object.angle +=
            quantize(
                object.angularVelocity * delta,
                object.mechanicsRotation.step
            );
    }
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
