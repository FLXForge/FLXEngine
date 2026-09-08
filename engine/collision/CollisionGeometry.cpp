#include "CollisionGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
    constexpr float Epsilon = 0.0001f;

    float dot(Vector2 a, Vector2 b)
    {
        return a.x * b.x + a.y * b.y;
    }

    float length(Vector2 value)
    {
        return std::sqrt(dot(value, value));
    }

    Vector2 normalize(Vector2 value)
    {
        const float currentLength =
            length(value);

        if (currentLength <= Epsilon)
        {
            return Vector2{ 1.0f, 0.0f };
        }

        return Vector2{
            value.x / currentLength,
            value.y / currentLength
        };
    }

    Vector2 rotate(Vector2 value, float angle)
    {
        const float radians =
            angle * DEG2RAD;

        const float cosine =
            std::cos(radians);

        const float sine =
            std::sin(radians);

        return Vector2{
            value.x * cosine - value.y * sine,
            value.x * sine + value.y * cosine
        };
    }

    Vector2 axisX(const EffectiveCollider& collider)
    {
        return rotate(Vector2{ 1.0f, 0.0f }, collider.angle);
    }

    Vector2 axisY(const EffectiveCollider& collider)
    {
        return rotate(Vector2{ 0.0f, 1.0f }, collider.angle);
    }

    bool broadIntersects(Rectangle a, Rectangle b)
    {
        return
            a.x <= b.x + b.width &&
            a.x + a.width >= b.x &&
            a.y <= b.y + b.height &&
            a.y + a.height >= b.y;
    }

    float supportRadius(
        const EffectiveCollider& collider,
        Vector2 axis
    )
    {
        axis = normalize(axis);

        if (collider.type == "ellipse")
        {
            const float localX =
                dot(axis, axisX(collider));

            const float localY =
                dot(axis, axisY(collider));

            return std::sqrt(
                collider.halfSize.x * collider.halfSize.x * localX * localX +
                collider.halfSize.y * collider.halfSize.y * localY * localY
            );
        }

        return
            std::abs(dot(axis, axisX(collider))) * collider.halfSize.x +
            std::abs(dot(axis, axisY(collider))) * collider.halfSize.y;
    }

    void addBoxAxes(
        const EffectiveCollider& collider,
        std::vector<Vector2>& axes
    )
    {
        axes.push_back(axisX(collider));
        axes.push_back(axisY(collider));
    }

    bool satContact(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        Vector2& normal,
        float& penetration
    )
    {
        std::vector<Vector2> axes;

        if (source.type == "box")
        {
            addBoxAxes(source, axes);
        }

        if (target.type == "box")
        {
            addBoxAxes(target, axes);
        }

        const Vector2 centerDelta{
            source.center.x - target.center.x,
            source.center.y - target.center.y
        };

        if (source.type == "ellipse" || target.type == "ellipse")
        {
            axes.push_back(normalize(centerDelta));
        }

        if (axes.empty())
        {
            axes.push_back(normalize(centerDelta));
        }

        penetration =
            std::numeric_limits<float>::max();
        normal =
            Vector2{ 1.0f, 0.0f };

        for (Vector2 axis : axes)
        {
            axis =
                normalize(axis);

            const float distance =
                std::abs(dot(centerDelta, axis));

            const float overlap =
                supportRadius(source, axis) +
                supportRadius(target, axis) -
                distance;

            if (overlap < 0.0f)
            {
                return false;
            }

            if (overlap < penetration)
            {
                penetration =
                    overlap;

                normal =
                    dot(centerDelta, axis) >= 0.0f
                    ? axis
                    : Vector2{ -axis.x, -axis.y };
            }
        }

        return true;
    }

    Vector2 closestPointOnBox(
        const EffectiveCollider& box,
        Vector2 point
    )
    {
        const Vector2 localDelta{
            point.x - box.center.x,
            point.y - box.center.y
        };

        const Vector2 xAxis =
            axisX(box);

        const Vector2 yAxis =
            axisY(box);

        const float localX =
            std::clamp(dot(localDelta, xAxis), -box.halfSize.x, box.halfSize.x);

        const float localY =
            std::clamp(dot(localDelta, yAxis), -box.halfSize.y, box.halfSize.y);

        return Vector2{
            box.center.x + xAxis.x * localX + yAxis.x * localY,
            box.center.y + xAxis.y * localX + yAxis.y * localY
        };
    }

    bool rayBox(
        const EffectiveCollider& box,
        Vector2 origin,
        Vector2 direction,
        float maxDistance,
        RayCollisionHit& hit
    )
    {
        const Vector2 xAxis =
            axisX(box);

        const Vector2 yAxis =
            axisY(box);

        const Vector2 delta{
            origin.x - box.center.x,
            origin.y - box.center.y
        };

        const Vector2 localOrigin{
            dot(delta, xAxis),
            dot(delta, yAxis)
        };

        const Vector2 localDirection{
            dot(direction, xAxis),
            dot(direction, yAxis)
        };

        float tMin = 0.0f;
        float tMax = maxDistance;
        Vector2 normal = Vector2{ 0.0f, 0.0f };

        const auto slab =
            [&](float originAxis, float directionAxis, float half, Vector2 axisNormal)
            {
                if (std::abs(directionAxis) <= Epsilon)
                {
                    return originAxis >= -half && originAxis <= half;
                }

                float t1 =
                    (-half - originAxis) / directionAxis;

                float t2 =
                    (half - originAxis) / directionAxis;

                Vector2 enterNormal =
                    axisNormal;

                if (t1 > t2)
                {
                    std::swap(t1, t2);
                    enterNormal = Vector2{ -axisNormal.x, -axisNormal.y };
                }

                if (t1 > tMin)
                {
                    tMin = t1;
                    normal = enterNormal;
                }

                tMax = std::min(tMax, t2);

                return tMin <= tMax;
            };

        if (!slab(localOrigin.x, localDirection.x, box.halfSize.x, Vector2{ -xAxis.x, -xAxis.y }) ||
            !slab(localOrigin.y, localDirection.y, box.halfSize.y, Vector2{ -yAxis.x, -yAxis.y }))
        {
            return false;
        }

        if (tMin < 0.0f)
        {
            tMin = tMax;
            normal = normalize(Vector2{
                origin.x - box.center.x,
                origin.y - box.center.y
            });
        }

        if (tMin < 0.0f || tMin > maxDistance)
        {
            return false;
        }

        hit.hit = true;
        hit.distance = tMin;
        hit.point = Vector2{
            origin.x + direction.x * tMin,
            origin.y + direction.y * tMin
        };
        hit.normal = normalize(normal);
        return true;
    }

    bool rayEllipse(
        const EffectiveCollider& ellipse,
        Vector2 origin,
        Vector2 direction,
        float maxDistance,
        RayCollisionHit& hit
    )
    {
        const Vector2 xAxis =
            axisX(ellipse);

        const Vector2 yAxis =
            axisY(ellipse);

        const Vector2 delta{
            origin.x - ellipse.center.x,
            origin.y - ellipse.center.y
        };

        const Vector2 localOrigin{
            dot(delta, xAxis) / ellipse.halfSize.x,
            dot(delta, yAxis) / ellipse.halfSize.y
        };

        const Vector2 localDirection{
            dot(direction, xAxis) / ellipse.halfSize.x,
            dot(direction, yAxis) / ellipse.halfSize.y
        };

        const float a =
            dot(localDirection, localDirection);

        const float b =
            2.0f * dot(localOrigin, localDirection);

        const float c =
            dot(localOrigin, localOrigin) - 1.0f;

        const float discriminant =
            b * b - 4.0f * a * c;

        if (a <= Epsilon || discriminant < 0.0f)
        {
            return false;
        }

        const float root =
            std::sqrt(std::max(0.0f, discriminant));

        const float t1 =
            (-b - root) / (2.0f * a);

        const float t2 =
            (-b + root) / (2.0f * a);

        const float t =
            t1 >= 0.0f ? t1 : t2;

        if (t < 0.0f || t > maxDistance)
        {
            return false;
        }

        hit.hit = true;
        hit.distance = t;
        hit.point = Vector2{
            origin.x + direction.x * t,
            origin.y + direction.y * t
        };

        const Vector2 pointDelta{
            hit.point.x - ellipse.center.x,
            hit.point.y - ellipse.center.y
        };

        const Vector2 localPoint{
            dot(pointDelta, xAxis) / ellipse.halfSize.x,
            dot(pointDelta, yAxis) / ellipse.halfSize.y
        };

        Vector2 worldNormal{
            xAxis.x * localPoint.x / ellipse.halfSize.x +
                yAxis.x * localPoint.y / ellipse.halfSize.y,
            xAxis.y * localPoint.x / ellipse.halfSize.x +
                yAxis.y * localPoint.y / ellipse.halfSize.y
        };

        hit.normal =
            normalize(worldNormal);

        return true;
    }
}

bool CollisionGeometry::contact(
    const EffectiveCollider& source,
    const EffectiveCollider& target,
    CollisionContact& result
)
{
    if (!source.effective || !target.effective)
    {
        return false;
    }

    if (!broadIntersects(source.broadBounds, target.broadBounds))
    {
        return false;
    }

    Vector2 normal;
    float penetration = 0.0f;

    if (!satContact(source, target, normal, penetration))
    {
        return false;
    }

    result.collider = source.name;
    result.otherCollider = target.name;
    result.normal = normal;
    result.penetration = std::max(0.0f, penetration);

    const Vector2 sourcePoint =
        target.type == "box"
        ? closestPointOnBox(target, source.center)
        : Vector2{
            target.center.x + normal.x * target.halfSize.x,
            target.center.y + normal.y * target.halfSize.y
        };

    const Vector2 targetPoint =
        source.type == "box"
        ? closestPointOnBox(source, target.center)
        : Vector2{
            source.center.x - normal.x * source.halfSize.x,
            source.center.y - normal.y * source.halfSize.y
        };

    result.point = Vector2{
        (sourcePoint.x + targetPoint.x) / 2.0f,
        (sourcePoint.y + targetPoint.y) / 2.0f
    };

    return true;
}

RayCollisionHit CollisionGeometry::ray(
    const EffectiveCollider& target,
    Vector2 origin,
    Vector2 direction,
    float maxDistance
)
{
    RayCollisionHit hit;

    if (!target.effective || maxDistance <= 0.0f)
    {
        return hit;
    }

    direction =
        normalize(direction);

    if (target.type == "ellipse")
    {
        rayEllipse(target, origin, direction, maxDistance, hit);
        return hit;
    }

    rayBox(target, origin, direction, maxDistance, hit);
    return hit;
}
