#pragma once

#include "VisualPrimitive.h"

#include <vector>

class RuntimeObject;
struct RepresentationRenderContext;
class Diagnostics;

class RepresentationPrimitiveBuilder
{
public:
    // Production capture: validates the complete layer and recovers per instance.
    static std::vector<VisualPrimitive> build(RuntimeObject& object, Color fallbackColor,
        const RepresentationRenderContext& context, Diagnostics& diagnostics);
    // Raw snapshot helper; viewport-aware recovery requires the overload above.
    static std::vector<VisualPrimitive> build(
        const RuntimeObject& object,
        Color fallbackColor
    );
};
