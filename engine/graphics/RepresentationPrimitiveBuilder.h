#pragma once

#include "VisualPrimitive.h"

#include <vector>

class RuntimeObject;

class RepresentationPrimitiveBuilder
{
public:
    static std::vector<VisualPrimitive> build(
        const RuntimeObject& object,
        Color fallbackColor
    );
};
