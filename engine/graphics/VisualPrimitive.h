#pragma once

#include <raylib.h>

#include <string>
#include <vector>

enum class VisualPrimitiveKind
{
    Pixel,
    Line,
    Rectangle,
    Text,
    Representation
};

struct VisualPrimitive
{
    VisualPrimitiveKind kind = VisualPrimitiveKind::Pixel;
    std::string ownerRuntimeId;
    std::string presentationSourceRuntimeId;
    bool presentationWrap = false;
    bool presentationOverflow = false;
    Vector2 presentationPosition = Vector2{ 0.0f, 0.0f };
    Vector2 presentationSize = Vector2{ 0.0f, 0.0f };

    Vector2 a = Vector2{ 0.0f, 0.0f };
    Vector2 b = Vector2{ 0.0f, 0.0f };
    Vector2 size = Vector2{ 0.0f, 0.0f };
    float angle = 0.0f;
    int fontSize = 10;
    Color color = WHITE;

    std::string primitive;
    std::string primitiveMode = "fill";
    std::string geometryMode = "open";
    std::string text;
    std::vector<Vector2> points;
};
