#include "DrawingContext.h"

#include "../debug/Logger.h"
#include "../runtime/RuntimeObject.h"

#include <cmath>

namespace
{
    Vector2 rotateLocal(Vector2 point, float angle)
    {
        const float radians =
            angle * DEG2RAD;

        const float cosine =
            std::cos(radians);

        const float sine =
            std::sin(radians);

        return Vector2{
            point.x * cosine - point.y * sine,
            point.x * sine + point.y * cosine
        };
    }

    Vector2 localToWorld(
        const RuntimeObject& reference,
        Vector2 local
    )
    {
        const Vector2 rotated =
            rotateLocal(local, reference.angle);

        return Vector2{
            reference.position.x + rotated.x,
            reference.position.y + rotated.y
        };
    }

    void capturePresentation(
        VisualPrimitive& primitive,
        const RuntimeObject& reference
    )
    {
        primitive.presentationSourceRuntimeId =
            reference.runtimeId;
        primitive.presentationWrap =
            reference.boundsMode == "wrap";
        primitive.presentationOverflow =
            reference.boundsOverflow;
        primitive.presentationPosition =
            reference.position;
        primitive.presentationSize =
            reference.size;
    }
}

void DrawingContext::begin(
    RuntimeObject& owner,
    std::vector<VisualPrimitive>& target
)
{
    currentOwner =
        &owner;

    primitives =
        &target;
}

void DrawingContext::end()
{
    currentOwner =
        nullptr;

    primitives =
        nullptr;
}

bool DrawingContext::active() const
{
    return currentOwner != nullptr && primitives != nullptr;
}

const RuntimeObject* DrawingContext::owner() const
{
    return currentOwner;
}

bool DrawingContext::canEmit() const
{
    if (active())
    {
        return true;
    }

    Logger::warning(
        "graphics",
        "Immediate drawing requires an active draw() context"
    );

    return false;
}

VisualPrimitive DrawingContext::basePrimitive(
    VisualPrimitiveKind kind,
    Color color
) const
{
    VisualPrimitive primitive;
    primitive.kind =
        kind;
    primitive.color =
        color;

    if (currentOwner != nullptr)
    {
        primitive.ownerRuntimeId =
            currentOwner->runtimeId;
    }

    return primitive;
}

bool DrawingContext::emitWorldPixel(Vector2 point, Color color)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Pixel, color);
    primitive.a =
        point;

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitLocalPixel(
    const RuntimeObject& reference,
    Vector2 point,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Pixel, color);
    capturePresentation(
        primitive,
        reference
    );
    primitive.a =
        localToWorld(reference, point);

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitWorldLine(
    Vector2 start,
    Vector2 end,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Line, color);
    primitive.a =
        start;
    primitive.b =
        end;

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitLocalLine(
    const RuntimeObject& reference,
    Vector2 start,
    Vector2 end,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Line, color);
    capturePresentation(
        primitive,
        reference
    );
    primitive.a =
        localToWorld(reference, start);
    primitive.b =
        localToWorld(reference, end);

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitWorldRectangle(
    Vector2 center,
    Vector2 size,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Rectangle, color);
    primitive.a =
        center;
    primitive.size =
        size;

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitLocalRectangle(
    const RuntimeObject& reference,
    Vector2 center,
    Vector2 size,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Rectangle, color);
    capturePresentation(
        primitive,
        reference
    );
    primitive.a =
        localToWorld(reference, center);
    primitive.size =
        size;
    primitive.angle =
        reference.angle;

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitWorldText(
    Vector2 center,
    const std::string& text,
    int fontSize,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Text, color);
    primitive.a =
        center;
    primitive.fontSize =
        fontSize;
    primitive.text =
        text;

    primitives->push_back(primitive);
    return true;
}

bool DrawingContext::emitLocalText(
    const RuntimeObject& reference,
    Vector2 center,
    const std::string& text,
    int fontSize,
    Color color
)
{
    if (!canEmit())
    {
        return false;
    }

    VisualPrimitive primitive =
        basePrimitive(VisualPrimitiveKind::Text, color);
    capturePresentation(
        primitive,
        reference
    );
    primitive.a =
        localToWorld(reference, center);
    primitive.fontSize =
        fontSize;
    primitive.text =
        text;
    primitive.angle =
        reference.angle;

    primitives->push_back(primitive);
    return true;
}

