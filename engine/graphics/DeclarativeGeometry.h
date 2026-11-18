#pragma once
#include "VisualPrimitive.h"
#include "../runtime/WorldExtent.h"
#include <array>

struct RepresentationRenderContext
{
    WorldExtent world;
    float width = 0, height = 0;
    Font font{};
};

struct DeclarativeQuad
{
    std::array<Vector2, 4> points;
    std::array<Vector2, 4> uv;
};

struct DeclarativeMesh
{
    std::vector<Vector2> points;
    std::vector<DeclarativeQuad> glyphs;
    Vector2 center{};
    float lineWidth = 1;
    unsigned int fontTexture = 0;
    bool transformed = false;
    bool collapsed = false;
    bool singleCell = false;
};

namespace DeclarativeGeometry
{
    bool identity(const AnimationTransform& transform);
    std::vector<Vector2> presentationOffsets(const VisualPrimitive& primitive, const WorldExtent& world);
    // CPU geometry only. Font metrics are supplied by the caller, never loaded here.
    bool prepare(const VisualPrimitive& primitive, const RepresentationRenderContext& context,
        Vector2 presentationOffset, DeclarativeMesh& mesh);
    bool worldPoint(const VisualPrimitive& primitive, double x, double y, Vector2& world);
}
