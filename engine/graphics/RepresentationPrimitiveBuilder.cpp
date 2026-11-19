#include "RepresentationPrimitiveBuilder.h"

#include "../runtime/RuntimeObject.h"

#include <algorithm>
#include "DeclarativeGeometry.h"

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

static std::vector<VisualPrimitive> buildAppearance(
    const RuntimeObject& object,
    const AnimationAppearance& appearance,
    const AnimationRepresentation* baseElements = nullptr
)
{
    std::vector<VisualPrimitive> primitives;
    const AnimationRepresentation* groups[] = {
        appearance.first.get(), appearance.replaceable.get(), appearance.last.get()
    };
    constexpr RepresentationPersistence partitions[] = {
        RepresentationPersistence::First, RepresentationPersistence::None, RepresentationPersistence::Last
    };
    size_t count = baseElements ? baseElements->size() : 0;
    if (!baseElements)
        for (const auto* group : groups) if (group) count += group->size();
    primitives.reserve(count);

    // ABSENT borrows the base only for this capture. Output primitives still own
    // their snapshot data; stable persist ordering needs no copied partitions.
    for (size_t index = 0; index < 3; ++index)
    {
        const auto* group = baseElements ? baseElements : groups[index];
        if (!group) continue;
        for (const RepresentationElementDefinition& element : *group)
        {
            if (baseElements && element.persist != partitions[index]) continue;
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
                    : appearance.color;
            primitive.visualTransform = appearance.transform;

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
    }
    return primitives;
}

std::vector<VisualPrimitive> RepresentationPrimitiveBuilder::build(const RuntimeObject& object, Color fallbackColor)
{
    if (object.animation.status() == AnimationPlaybackStatus::Absent)
    {
        AnimationAppearance base;
        base.color = object.hasVisualColor ? object.visualColor : fallbackColor;
        return buildAppearance(object, base, &object.representation);
    }
    const auto base=AnimationBaseAppearance::capture(object.representation,
        object.hasVisualColor?object.visualColor:fallbackColor,object.hasVisualColor);
    return buildAppearance(object,object.animation.appearance(base));
}

std::vector<VisualPrimitive> RepresentationPrimitiveBuilder::build(RuntimeObject& object, Color fallbackColor,
    const RepresentationRenderContext& context, Diagnostics& diagnostics)
{
    const bool absent = object.animation.status() == AnimationPlaybackStatus::Absent;
    AnimationBaseAppearance base;
    base.color = object.hasVisualColor ? object.visualColor : fallbackColor;
    base.hasRuntimeColor = object.hasVisualColor;
    if (!absent)
        base = AnimationBaseAppearance::capture(object.representation, base.color, base.hasRuntimeColor);
    std::vector<VisualPrimitive> result;
    const bool valid=object.animation.validateAppearance(base,[&](const AnimationAppearance& appearance) {
        auto candidate=buildAppearance(object,appearance,absent ? &object.representation : nullptr);
        for(auto& primitive:candidate)
        {
            for(const auto offset:DeclarativeGeometry::presentationOffsets(primitive,context.world))
            {
                auto mesh=std::make_shared<DeclarativeMesh>();
                if(!DeclarativeGeometry::prepare(primitive,context,offset,*mesh))return false;
                primitive.presentationMeshes.push_back(mesh);
            }
            primitive.mesh=primitive.presentationMeshes.front();
        }
        result=std::move(candidate);return true;
    },diagnostics);
    if(!valid)result.clear();
    return result;
}
