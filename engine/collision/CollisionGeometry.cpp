#include "CollisionGeometry.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
    constexpr float Epsilon = 0.0001f;
    constexpr int MaxGjkIterations = 32;
    constexpr int MaxEpaIterations = 48;
    constexpr float EpaTolerance = 0.0005f;

    struct SupportPoint
    {
        Vector2 point = Vector2{ 0.0f, 0.0f };
        Vector2 source = Vector2{ 0.0f, 0.0f };
        Vector2 target = Vector2{ 0.0f, 0.0f };
    };

    struct ContactResult
    {
        bool hit = false;
        Vector2 normal = Vector2{ 1.0f, 0.0f };
        Vector2 point = Vector2{ 0.0f, 0.0f };
        float penetration = 0.0f;
    };

    struct EpaEdge
    {
        float distance = std::numeric_limits<float>::max();
        Vector2 normal = Vector2{ 1.0f, 0.0f };
        std::size_t index = 0;
    };

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

    Vector2 negate(Vector2 value)
    {
        return Vector2{ -value.x, -value.y };
    }

    Vector2 subtract(Vector2 a, Vector2 b)
    {
        return Vector2{
            a.x - b.x,
            a.y - b.y
        };
    }

    Vector2 add(Vector2 a, Vector2 b)
    {
        return Vector2{
            a.x + b.x,
            a.y + b.y
        };
    }

    Vector2 multiply(Vector2 value, float factor)
    {
        return Vector2{
            value.x * factor,
            value.y * factor
        };
    }

    float clamp01(float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    Vector2 lerp(Vector2 a, Vector2 b, float amount)
    {
        return Vector2{
            a.x + (b.x - a.x) * amount,
            a.y + (b.y - a.y) * amount
        };
    }

    bool sameDirection(Vector2 a, Vector2 b)
    {
        return dot(a, b) > 0.0f;
    }

    Vector2 perpendicularTowardOrigin(Vector2 edge, Vector2 toward)
    {
        Vector2 result = Vector2{
            edge.y,
            -edge.x
        };

        if (!sameDirection(result, toward))
        {
            result =
                negate(result);
        }

        if (length(result) <= Epsilon)
        {
            result = Vector2{
                -edge.y,
                edge.x
            };
        }

        return normalize(result);
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

    float cross(Vector2 a, Vector2 b)
    {
        return a.x * b.y - a.y * b.x;
    }

    Vector2 tripleProduct(Vector2 a, Vector2 b, Vector2 c)
    {
        const float ac =
            dot(a, c);

        const float bc =
            dot(b, c);

        return Vector2{
            b.x * ac - a.x * bc,
            b.y * ac - a.y * bc
        };
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

    Vector2 supportPoint(
        const EffectiveCollider& collider,
        Vector2 direction
    )
    {
        direction =
            normalize(direction);

        const Vector2 xAxis =
            axisX(collider);

        const Vector2 yAxis =
            axisY(collider);

        if (collider.type == "ellipse")
        {
            const float localX =
                dot(direction, xAxis);

            const float localY =
                dot(direction, yAxis);

            const float scaledX =
                collider.halfSize.x * localX;

            const float scaledY =
                collider.halfSize.y * localY;

            const float denominator =
                std::sqrt(scaledX * scaledX + scaledY * scaledY);

            if (denominator <= Epsilon)
            {
                return add(
                    collider.center,
                    multiply(xAxis, collider.halfSize.x)
                );
            }

            return Vector2{
                collider.center.x +
                    xAxis.x * collider.halfSize.x * scaledX / denominator +
                    yAxis.x * collider.halfSize.y * scaledY / denominator,
                collider.center.y +
                    xAxis.y * collider.halfSize.x * scaledX / denominator +
                    yAxis.y * collider.halfSize.y * scaledY / denominator
            };
        }

        return Vector2{
            collider.center.x +
                xAxis.x * (dot(direction, xAxis) >= 0.0f ? collider.halfSize.x : -collider.halfSize.x) +
                yAxis.x * (dot(direction, yAxis) >= 0.0f ? collider.halfSize.y : -collider.halfSize.y),
            collider.center.y +
                xAxis.y * (dot(direction, xAxis) >= 0.0f ? collider.halfSize.x : -collider.halfSize.x) +
                yAxis.y * (dot(direction, yAxis) >= 0.0f ? collider.halfSize.y : -collider.halfSize.y)
        };
    }

    SupportPoint support(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        Vector2 direction
    )
    {
        SupportPoint point;
        point.source =
            supportPoint(source, direction);
        point.target =
            supportPoint(target, negate(direction));
        point.point =
            subtract(point.source, point.target);
        return point;
    }

    void addBoxAxes(
        const EffectiveCollider& collider,
        std::vector<Vector2>& axes
    )
    {
        axes.push_back(axisX(collider));
        axes.push_back(axisY(collider));
    }

    std::array<Vector2, 4> boxCorners(const EffectiveCollider& collider)
    {
        const Vector2 xAxis =
            axisX(collider);

        const Vector2 yAxis =
            axisY(collider);

        const Vector2 xExtent =
            multiply(xAxis, collider.halfSize.x);

        const Vector2 yExtent =
            multiply(yAxis, collider.halfSize.y);

        return std::array<Vector2, 4>{
            add(add(collider.center, negate(xExtent)), negate(yExtent)),
            add(add(collider.center, xExtent), negate(yExtent)),
            add(add(collider.center, xExtent), yExtent),
            add(add(collider.center, negate(xExtent)), yExtent)
        };
    }

    bool pointInsideBox(
        Vector2 point,
        const EffectiveCollider& box
    )
    {
        const Vector2 delta =
            subtract(point, box.center);

        return
            std::abs(dot(delta, axisX(box))) <= box.halfSize.x + Epsilon &&
            std::abs(dot(delta, axisY(box))) <= box.halfSize.y + Epsilon;
    }

    void addUniquePoint(
        std::vector<Vector2>& points,
        Vector2 point
    )
    {
        constexpr float PointEpsilon = 0.01f;

        for (Vector2 existing : points)
        {
            if (length(subtract(existing, point)) <= PointEpsilon)
            {
                return;
            }
        }

        points.push_back(point);
    }

    bool segmentIntersection(
        Vector2 a,
        Vector2 b,
        Vector2 c,
        Vector2 d,
        Vector2& point
    )
    {
        const Vector2 r =
            subtract(b, a);

        const Vector2 s =
            subtract(d, c);

        const float denominator =
            cross(r, s);

        if (std::abs(denominator) <= Epsilon)
        {
            return false;
        }

        const Vector2 offset =
            subtract(c, a);

        const float amountA =
            cross(offset, s) / denominator;

        const float amountB =
            cross(offset, r) / denominator;

        if (
            amountA < -Epsilon ||
            amountA > 1.0f + Epsilon ||
            amountB < -Epsilon ||
            amountB > 1.0f + Epsilon
        )
        {
            return false;
        }

        point =
            add(a, multiply(r, clamp01(amountA)));

        return true;
    }

    bool boxIntersectionContactPoint(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        Vector2& point
    )
    {
        const std::array<Vector2, 4> sourceCorners =
            boxCorners(source);

        const std::array<Vector2, 4> targetCorners =
            boxCorners(target);

        std::vector<Vector2> intersectionPoints;

        for (Vector2 corner : sourceCorners)
        {
            if (pointInsideBox(corner, target))
            {
                addUniquePoint(intersectionPoints, corner);
            }
        }

        for (Vector2 corner : targetCorners)
        {
            if (pointInsideBox(corner, source))
            {
                addUniquePoint(intersectionPoints, corner);
            }
        }

        for (std::size_t sourceIndex = 0; sourceIndex < sourceCorners.size(); ++sourceIndex)
        {
            const Vector2 sourceA =
                sourceCorners[sourceIndex];

            const Vector2 sourceB =
                sourceCorners[(sourceIndex + 1) % sourceCorners.size()];

            for (std::size_t targetIndex = 0; targetIndex < targetCorners.size(); ++targetIndex)
            {
                const Vector2 targetA =
                    targetCorners[targetIndex];

                const Vector2 targetB =
                    targetCorners[(targetIndex + 1) % targetCorners.size()];

                Vector2 intersection =
                    Vector2{ 0.0f, 0.0f };

                if (segmentIntersection(
                    sourceA,
                    sourceB,
                    targetA,
                    targetB,
                    intersection
                ))
                {
                    addUniquePoint(intersectionPoints, intersection);
                }
            }
        }

        if (intersectionPoints.empty())
        {
            return false;
        }

        Vector2 sum =
            Vector2{ 0.0f, 0.0f };

        for (Vector2 intersection : intersectionPoints)
        {
            sum =
                add(sum, intersection);
        }

        point =
            multiply(
                sum,
                1.0f / static_cast<float>(intersectionPoints.size())
            );

        return true;
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

    bool handleSimplex(
        std::vector<SupportPoint>& simplex,
        Vector2& direction
    )
    {
        const SupportPoint& a =
            simplex.back();

        const Vector2 ao =
            negate(a.point);

        if (simplex.size() == 2)
        {
            const Vector2 ab =
                subtract(simplex[0].point, a.point);

            if (sameDirection(ab, ao))
            {
                direction =
                    tripleProduct(ab, ao, ab);

                if (length(direction) <= Epsilon)
                {
                    direction =
                        perpendicularTowardOrigin(ab, ao);
                }
            }
            else
            {
                simplex.erase(simplex.begin());
                direction =
                    ao;
            }

            return false;
        }

        if (simplex.size() == 3)
        {
            const Vector2 ab =
                subtract(simplex[1].point, a.point);

            const Vector2 ac =
                subtract(simplex[0].point, a.point);

            const Vector2 abPerp =
                tripleProduct(ac, ab, ab);

            if (sameDirection(abPerp, ao))
            {
                simplex.erase(simplex.begin());
                direction =
                    length(abPerp) <= Epsilon
                    ? perpendicularTowardOrigin(ab, ao)
                    : abPerp;
                return false;
            }

            const Vector2 acPerp =
                tripleProduct(ab, ac, ac);

            if (sameDirection(acPerp, ao))
            {
                simplex.erase(simplex.begin() + 1);
                direction =
                    length(acPerp) <= Epsilon
                    ? perpendicularTowardOrigin(ac, ao)
                    : acPerp;
                return false;
            }

            return true;
        }

        direction =
            ao;

        return false;
    }

    bool gjk(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        std::vector<SupportPoint>& simplex
    )
    {
        Vector2 direction =
            subtract(source.center, target.center);

        if (length(direction) <= Epsilon)
        {
            direction = Vector2{ 1.0f, 0.0f };
        }

        simplex.clear();
        simplex.push_back(
            support(source, target, direction)
        );

        direction =
            negate(simplex.back().point);

        for (int i = 0; i < MaxGjkIterations; ++i)
        {
            SupportPoint next =
                support(source, target, direction);

            if (dot(next.point, direction) < -Epsilon)
            {
                return false;
            }

            simplex.push_back(next);

            if (handleSimplex(simplex, direction))
            {
                return true;
            }

            if (length(direction) <= Epsilon)
            {
                return true;
            }
        }

        return false;
    }

    float polygonArea(
        const std::vector<SupportPoint>& points
    )
    {
        float area = 0.0f;

        for (std::size_t i = 0; i < points.size(); ++i)
        {
            const Vector2 a =
                points[i].point;

            const Vector2 b =
                points[(i + 1) % points.size()].point;

            area +=
                cross(a, b);
        }

        return area / 2.0f;
    }

    EpaEdge closestEpaEdge(
        const std::vector<SupportPoint>& simplex
    )
    {
        EpaEdge result;

        for (std::size_t i = 0; i < simplex.size(); ++i)
        {
            const Vector2 a =
                simplex[i].point;

            const Vector2 b =
                simplex[(i + 1) % simplex.size()].point;

            const Vector2 edge =
                subtract(b, a);

            Vector2 normal =
                normalize(Vector2{ edge.y, -edge.x });

            float distance =
                dot(normal, a);

            if (distance < 0.0f)
            {
                distance =
                    -distance;

                normal =
                    negate(normal);
            }

            if (distance < result.distance)
            {
                result.distance =
                    distance;

                result.normal =
                    normal;

                result.index =
                    i;
            }
        }

        return result;
    }

    Vector2 representativeContactPointFromEdge(
        const std::vector<SupportPoint>& simplex,
        std::size_t edgeIndex
    )
    {
        const SupportPoint& a =
            simplex[edgeIndex];

        const SupportPoint& b =
            simplex[(edgeIndex + 1) % simplex.size()];

        const Vector2 edge =
            subtract(b.point, a.point);

        const float edgeLengthSquared =
            dot(edge, edge);

        const float amount =
            edgeLengthSquared <= Epsilon
            ? 0.0f
            : clamp01(-dot(a.point, edge) / edgeLengthSquared);

        const Vector2 sourceWitness =
            lerp(a.source, b.source, amount);

        const Vector2 targetWitness =
            lerp(a.target, b.target, amount);

        return multiply(
            add(sourceWitness, targetWitness),
            0.5f
        );
    }

    ContactResult epa(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        std::vector<SupportPoint> simplex
    )
    {
        ContactResult result;

        if (simplex.size() < 3)
        {
            return result;
        }

        if (polygonArea(simplex) < 0.0f)
        {
            std::swap(simplex[0], simplex[1]);
        }

        for (int iteration = 0; iteration < MaxEpaIterations; ++iteration)
        {
            const EpaEdge bestEdge =
                closestEpaEdge(simplex);

            const SupportPoint point =
                support(source, target, bestEdge.normal);

            const float supportDistance =
                dot(bestEdge.normal, point.point);

            if (supportDistance - bestEdge.distance <= EpaTolerance)
            {
                Vector2 finalNormal =
                    bestEdge.normal;

                if (dot(
                    finalNormal,
                    subtract(source.center, target.center)
                ) < 0.0f)
                {
                    finalNormal =
                        negate(finalNormal);
                }

                result.hit = true;
                result.normal =
                    finalNormal;
                result.penetration =
                    std::max(0.0f, supportDistance);

                result.point =
                    representativeContactPointFromEdge(
                        simplex,
                        bestEdge.index
                    );

                return result;
            }

            simplex.insert(
                simplex.begin() + static_cast<std::ptrdiff_t>(bestEdge.index + 1),
                point
            );
        }

        const EpaEdge bestEdge =
            closestEpaEdge(simplex);

        Vector2 finalNormal =
            bestEdge.normal;

        if (dot(
            finalNormal,
            subtract(source.center, target.center)
        ) < 0.0f)
        {
            finalNormal =
                negate(finalNormal);
        }

        result.hit = true;
        result.normal =
            finalNormal;
        result.penetration =
            std::max(0.0f, bestEdge.distance);
        result.point =
            representativeContactPointFromEdge(
                simplex,
                bestEdge.index
            );

        return result;
    }

    bool supportContact(
        const EffectiveCollider& source,
        const EffectiveCollider& target,
        ContactResult& result
    )
    {
        std::vector<SupportPoint> simplex;

        if (!gjk(source, target, simplex))
        {
            return false;
        }

        result =
            epa(source, target, simplex);

        return result.hit;
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

    ContactResult contact;

    if (source.type == "box" && target.type == "box")
    {
        if (!satContact(
            source,
            target,
            contact.normal,
            contact.penetration
        ))
        {
            return false;
        }

        contact.hit =
            true;

        if (!boxIntersectionContactPoint(source, target, contact.point))
        {
            contact.point =
                multiply(
                    add(source.center, target.center),
                    0.5f
                );
        }
    }
    else if (!supportContact(source, target, contact))
    {
        return false;
    }

    result.collider = source.name;
    result.otherCollider = target.name;
    result.normal = contact.normal;
    result.penetration = contact.penetration;
    result.point = contact.point;

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
