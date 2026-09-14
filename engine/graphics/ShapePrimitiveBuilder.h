#pragma once

#include "VisualPrimitive.h"

class RuntimeObject;

class ShapePrimitiveBuilder
{
public:
    static bool build(
        const RuntimeObject& object,
        VisualPrimitive& primitive
    );
};

