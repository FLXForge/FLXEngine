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
    const AnimationAppearance& appearance
)
{
    std::vector<VisualPrimitive> primitives;
    size_t count=0;
    for(const auto& group:{appearance.first,appearance.replaceable,appearance.last})if(group)count+=group->size();
    primitives.reserve(count);

    for(const auto& group:{appearance.first,appearance.replaceable,appearance.last})
    {
        if(!group)continue;
        for (const RepresentationElementDefinition& element : *group)
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
    const auto base=AnimationBaseAppearance::capture(object.representation,
        object.hasVisualColor?object.visualColor:fallbackColor,object.hasVisualColor);
    return buildAppearance(object,object.animation.appearance(base));
}

std::vector<VisualPrimitive> RepresentationPrimitiveBuilder::build(RuntimeObject& object, Color fallbackColor,
    const RepresentationRenderContext& context, Diagnostics& diagnostics)
{
    const auto base=AnimationBaseAppearance::capture(object.representation,
        object.hasVisualColor?object.visualColor:fallbackColor,object.hasVisualColor);
    std::vector<VisualPrimitive> result;
    const bool valid=object.animation.validateAppearance(base,[&](const AnimationAppearance& appearance) {
        auto candidate=buildAppearance(object,appearance);
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
