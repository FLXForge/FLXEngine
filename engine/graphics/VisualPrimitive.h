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
    Shape
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
    float radius = 0.0f;
    int fontSize = 10;
    Color color = WHITE;

    std::string shapeType;
    std::string shapeMode = "fill";
    std::string text;
    std::vector<Vector2> points;
};
