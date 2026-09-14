#pragma once

#include "VisualPrimitive.h"

#include <raylib.h>

#include <vector>

class RuntimeObject;

class DrawingRenderer
{
public:
    static void render(
        const std::vector<VisualPrimitive>& primitives,
        const std::vector<RuntimeObject>& objects,
        int scale,
        float screenWidth,
        float screenHeight
    );

    static Vector2 logicalToPhysical(
        Vector2 point,
        int scale
    );
};

