#include "EffectiveColliderBuilder.h"
#include "../runtime/RuntimeObject.h"
#include "../scripting/ScriptEngine.h"

#include <algorithm>
#include <cmath>

namespace
{
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

    Rectangle boundsForBox(Vector2 center, Vector2 halfSize, float angle)
    {
        const Vector2 corners[4] = {
            rotate(Vector2{ -halfSize.x, -halfSize.y }, angle),
            rotate(Vector2{ halfSize.x, -halfSize.y }, angle),
            rotate(Vector2{ halfSize.x, halfSize.y }, angle),
            rotate(Vector2{ -halfSize.x, halfSize.y }, angle)
        };

        float minX = center.x + corners[0].x;
        float maxX = minX;
        float minY = center.y + corners[0].y;
        float maxY = minY;

        for (const Vector2& corner : corners)
        {
            minX = std::min(minX, center.x + corner.x);
            maxX = std::max(maxX, center.x + corner.x);
            minY = std::min(minY, center.y + corner.y);
            maxY = std::max(maxY, center.y + corner.y);
        }

        return Rectangle{ minX, minY, maxX - minX, maxY - minY };
    }

    bool stateAllowed(
        const ColliderDefinition& collider,
        const RuntimeObject& object,
        ScriptEngine& scriptEngine
    )
    {
        if (collider.states.empty())
        {
            return true;
        }

        RuntimeObject* stateObject =
            scriptEngine.effectiveStateObject(
                const_cast<RuntimeObject&>(object)
            );

        if (stateObject == nullptr)
        {
            return false;
        }

        return std::find(
            collider.states.begin(),
            collider.states.end(),
            stateObject->state
        ) != collider.states.end();
    }
}

std::vector<EffectiveCollider> EffectiveColliderBuilder::build(
    const RuntimeObject& object,
    ScriptEngine& scriptEngine
)
{
    std::vector<EffectiveCollider> result;
    result.reserve(object.collisions.size());

    for (const auto& pair : object.collisions)
    {
        const ColliderDefinition& declaration =
            pair.second;

        const float width =
            declaration.size.hasWidth
            ? declaration.size.width
            : object.size.x;

        const float height =
            declaration.size.hasHeight
            ? declaration.size.height
            : object.size.y;

        EffectiveCollider collider;
        collider.owner = &object;
        collider.name = pair.first;
        collider.type = declaration.type;
        collider.with = declaration.with;
        collider.declaredEnabled = declaration.enabled;
        collider.stateAvailable =
            stateAllowed(declaration, object, scriptEngine);
        collider.effective =
            collider.declaredEnabled &&
            collider.stateAvailable &&
            width > 0.0f &&
            height > 0.0f;
        collider.halfSize = Vector2{ width / 2.0f, height / 2.0f };
        collider.angle = object.angle + declaration.angle;

        const Vector2 rotatedOffset =
            rotate(declaration.offset, object.angle);

        collider.center = Vector2{
            object.position.x + rotatedOffset.x,
            object.position.y + rotatedOffset.y
        };

        collider.broadBounds =
            boundsForBox(
                collider.center,
                collider.halfSize,
                collider.angle
            );

        result.push_back(collider);
    }

    return result;
}
