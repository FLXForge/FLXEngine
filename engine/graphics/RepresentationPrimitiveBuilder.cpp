#include "RepresentationPrimitiveBuilder.h"

#include "../runtime/RuntimeObject.h"

#include <algorithm>

namespace
{
    void capturePresentation(
        const RuntimeObject& object,
        VisualPrimitive& primitive
    )
    {
        primitive.ownerRuntimeId =
            object.runtimeId;
        primitive.presentationSourceRuntimeId =
            object.runtimeId;
        primitive.presentationWrap =
            object.boundsMode == "wrap";
        primitive.presentationOverflow =
            object.boundsOverflow;
        primitive.presentationPosition =
            object.position;
        primitive.presentationSize =
            object.size;
    }

    bool resolveAxis(
        const SizeAxisDefinition& axis,
        float objectAxis,
        bool objectHasSize,
        float& value
    )
    {
        if (axis.hasValue)
        {
            value =
                axis.percentage
                    ? objectAxis * axis.value
                    : axis.value;

            return true;
        }

        if (!objectHasSize)
        {
            return false;
        }

        value =
            objectAxis;

        return true;
    }

    bool resolvePrimitiveSize(
        const RuntimeObject& object,
        const RepresentationElementDefinition& element,
        Vector2& size
    )
    {
        return resolveAxis(
            element.size.width,
            object.size.x,
            object.hasSize,
            size.x
        ) &&
            resolveAxis(
                element.size.height,
                object.size.y,
                object.hasSize,
                size.y
            );
    }
}

std::vector<VisualPrimitive> RepresentationPrimitiveBuilder::build(
    const RuntimeObject& object,
    Color fallbackColor
)
{
    std::vector<VisualPrimitive> primitives;
    primitives.reserve(object.representation.size());

    for (const RepresentationElementDefinition& element : object.representation)
    {
        VisualPrimitive primitive;
        primitive.kind =
            VisualPrimitiveKind::Representation;
        capturePresentation(
            object,
            primitive
        );

        primitive.a =
            object.position;
        primitive.angle =
            object.angle;
        primitive.color =
            element.hasColor
                ? element.color
                : object.hasVisualColor
                    ? object.visualColor
                    : fallbackColor;

        if (element.kind == RepresentationElementKind::Bitmap)
        {
            if (!element.bitmap || !element.bitmap->valid()) continue;
            primitive.primitive = "bitmap";
            primitive.bitmap = element.bitmap;
            primitive.color = WHITE; // A bitmap is never tinted by object/element color.
            const float width = object.hasSize ? object.size.x : static_cast<float>(element.bitmap->width);
            const float height = object.hasSize ? object.size.y : static_cast<float>(element.bitmap->height);
            resolveAxis(element.size.width, width, true, primitive.size.x);
            resolveAxis(element.size.height, height, true, primitive.size.y);
            primitives.push_back(std::move(primitive));
            continue;
        }

        if (element.kind == RepresentationElementKind::Primitive)
        {
            primitive.primitive =
                element.primitive;
            primitive.primitiveMode =
                element.primitiveMode;

            if (!resolvePrimitiveSize(object, element, primitive.size))
            {
                continue;
            }

            primitives.push_back(primitive);
            continue;
        }

        if (element.kind == RepresentationElementKind::Geometry)
        {
            primitive.primitive =
                "geometry";
            primitive.geometryMode =
                element.geometryMode;
            primitive.points =
                element.geometry;
            primitives.push_back(primitive);
            continue;
        }

        if (element.kind == RepresentationElementKind::Text)
        {
            primitive.primitive =
                "text";
            primitive.text =
                element.text;
            primitive.fontSize =
                std::max(1, element.fontSize);
            primitives.push_back(primitive);
        }
    }

    return primitives;
}
