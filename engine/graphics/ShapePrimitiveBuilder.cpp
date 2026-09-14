#include "ShapePrimitiveBuilder.h"

#include "../runtime/RuntimeObject.h"

bool ShapePrimitiveBuilder::build(
    const RuntimeObject& object,
    VisualPrimitive& primitive
)
{
    if (object.shapeType == "none")
    {
        return false;
    }

    primitive = VisualPrimitive{};
    primitive.kind =
        VisualPrimitiveKind::Shape;
    primitive.ownerRuntimeId =
        object.runtimeId;
    primitive.presentationSourceRuntimeId =
        object.runtimeId;
    primitive.a =
        object.position;
    primitive.size =
        object.size;
    primitive.angle =
        object.angle;
    primitive.radius =
        object.radius;
    primitive.color =
        object.color;
    primitive.shapeType =
        object.shapeType;
    primitive.shapeMode =
        object.shapeMode;
    primitive.text =
        object.textContent;
    primitive.points =
        object.points;

    return true;
}

