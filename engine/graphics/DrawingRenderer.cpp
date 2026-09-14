#include "DrawingRenderer.h"

#include "../runtime/RuntimeObject.h"

#include <algorithm>
#include <cmath>

namespace
{
    bool isOutlineMode(const std::string& mode)
    {
        return mode == "outline";
    }

    Vector2 add(Vector2 left, Vector2 right)
    {
        return Vector2{
            left.x + right.x,
            left.y + right.y
        };
    }

    Vector2 rotateAround(
        Vector2 point,
        Vector2 center,
        float angleDegrees
    )
    {
        const float radians =
            angleDegrees * DEG2RAD;

        const float translatedX =
            point.x - center.x;

        const float translatedY =
            point.y - center.y;

        const float cosine =
            std::cos(radians);

        const float sine =
            std::sin(radians);

        return Vector2{
            center.x + translatedX * cosine - translatedY * sine,
            center.y + translatedX * sine + translatedY * cosine
        };
    }

    std::vector<Vector2> rectanglePoints(
        Vector2 center,
        Vector2 size,
        float angle
    )
    {
        std::vector<Vector2> points = {
            Vector2{ center.x - size.x / 2.0f, center.y - size.y / 2.0f },
            Vector2{ center.x + size.x / 2.0f, center.y - size.y / 2.0f },
            Vector2{ center.x + size.x / 2.0f, center.y + size.y / 2.0f },
            Vector2{ center.x - size.x / 2.0f, center.y + size.y / 2.0f }
        };

        for (Vector2& point : points)
        {
            point =
                rotateAround(point, center, angle);
        }

        return points;
    }

    Vector2 scalePoint(Vector2 point, int scale)
    {
        return Vector2{
            point.x * scale,
            point.y * scale
        };
    }

    std::vector<Vector2> presentationOffsets(
        const VisualPrimitive& primitive,
        const std::vector<RuntimeObject>& objects,
        float screenWidth,
        float screenHeight
    )
    {
        std::vector<Vector2> offsets = {
            Vector2{ 0.0f, 0.0f }
        };

        if (primitive.presentationSourceRuntimeId.empty())
        {
            return offsets;
        }

        const auto it =
            std::find_if(
                objects.begin(),
                objects.end(),
                [&primitive](const RuntimeObject& object)
                {
                    return object.runtimeId ==
                        primitive.presentationSourceRuntimeId;
                }
            );

        if (it == objects.end())
        {
            return offsets;
        }

        const RuntimeObject& source =
            *it;

        if (source.boundsMode != "wrap" || !source.boundsOverflow)
        {
            return offsets;
        }

        const float radius =
            std::sqrt(
                source.size.x * source.size.x +
                source.size.y * source.size.y
            ) / 2.0f;

        const bool overflowLeft =
            source.position.x - radius < 0.0f;

        const bool overflowRight =
            source.position.x + radius > screenWidth;

        const bool overflowTop =
            source.position.y - radius < 0.0f;

        const bool overflowBottom =
            source.position.y + radius > screenHeight;

        if (overflowLeft)
        {
            offsets.push_back(Vector2{ screenWidth, 0.0f });
        }

        if (overflowRight)
        {
            offsets.push_back(Vector2{ -screenWidth, 0.0f });
        }

        if (overflowTop)
        {
            offsets.push_back(Vector2{ 0.0f, screenHeight });
        }

        if (overflowBottom)
        {
            offsets.push_back(Vector2{ 0.0f, -screenHeight });
        }

        if (overflowLeft && overflowTop)
        {
            offsets.push_back(Vector2{ screenWidth, screenHeight });
        }

        if (overflowLeft && overflowBottom)
        {
            offsets.push_back(Vector2{ screenWidth, -screenHeight });
        }

        if (overflowRight && overflowTop)
        {
            offsets.push_back(Vector2{ -screenWidth, screenHeight });
        }

        if (overflowRight && overflowBottom)
        {
            offsets.push_back(Vector2{ -screenWidth, -screenHeight });
        }

        return offsets;
    }

    void drawPolygon(
        std::vector<Vector2> points,
        const std::string& mode,
        Color color,
        int scale
    )
    {
        if (points.size() < 2)
        {
            return;
        }

        for (Vector2& point : points)
        {
            point =
                scalePoint(point, scale);
        }

        if (!isOutlineMode(mode) && points.size() >= 3)
        {
            for (size_t i = 1; i + 1 < points.size(); ++i)
            {
                DrawTriangle(
                    points[0],
                    points[i + 1],
                    points[i],
                    color
                );
            }

            return;
        }

        for (size_t i = 0; i < points.size(); ++i)
        {
            DrawLineEx(
                points[i],
                points[(i + 1) % points.size()],
                std::max(1.0f, static_cast<float>(scale)),
                color
            );
        }
    }

    int fitFontSize(
        const std::string& text,
        float maxWidth,
        float maxHeight
    )
    {
        if (text.empty() || maxWidth <= 0.0f || maxHeight <= 0.0f)
        {
            return 1;
        }

        const Font font =
            GetFontDefault();

        constexpr float spacing = 1.0f;
        int low = 1;
        int high =
            std::max(1, static_cast<int>(maxHeight));
        int best = 1;

        while (low <= high)
        {
            const int middle =
                (low + high) / 2;

            const Vector2 measured =
                MeasureTextEx(
                    font,
                    text.c_str(),
                    static_cast<float>(middle),
                    spacing
                );

            if (measured.x <= maxWidth && measured.y <= maxHeight)
            {
                best = middle;
                low = middle + 1;
            }
            else
            {
                high = middle - 1;
            }
        }

        return best;
    }

    void drawCenteredText(
        const std::string& text,
        Vector2 center,
        int fontSize,
        float angle,
        Color color,
        int scale
    )
    {
        if (text.empty())
        {
            return;
        }

        const Font font =
            GetFontDefault();

        constexpr float spacing = 1.0f;

        const float scaledFontSize =
            static_cast<float>(fontSize * scale);

        const Vector2 measured =
            MeasureTextEx(
                font,
                text.c_str(),
                scaledFontSize,
                spacing * scale
            );

        DrawTextPro(
            font,
            text.c_str(),
            scalePoint(center, scale),
            Vector2{ measured.x / 2.0f, measured.y / 2.0f },
            angle,
            scaledFontSize,
            spacing * scale,
            color
        );
    }

    void drawShapeText(
        const VisualPrimitive& primitive,
        Vector2 center,
        int scale
    )
    {
        if (primitive.text.empty())
        {
            return;
        }

        const int fontSize =
            fitFontSize(
                primitive.text,
                primitive.size.x,
                primitive.size.y
            );

        if (isOutlineMode(primitive.shapeMode))
        {
            drawCenteredText(
                primitive.text,
                center,
                fontSize,
                primitive.angle,
                primitive.color,
                scale
            );

            drawCenteredText(
                primitive.text,
                center,
                std::max(1, static_cast<int>(fontSize * 0.82f)),
                primitive.angle,
                BLACK,
                scale
            );

            return;
        }

        drawCenteredText(
            primitive.text,
            center,
            fontSize,
            primitive.angle,
            primitive.color,
            scale
        );
    }

    void drawPrimitiveAt(
        const VisualPrimitive& primitive,
        Vector2 offset,
        int scale
    )
    {
        if (primitive.kind == VisualPrimitiveKind::Pixel)
        {
            const Vector2 point =
                scalePoint(add(primitive.a, offset), scale);

            DrawRectangle(
                static_cast<int>(std::round(point.x)),
                static_cast<int>(std::round(point.y)),
                std::max(1, scale),
                std::max(1, scale),
                primitive.color
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Line)
        {
            DrawLineEx(
                scalePoint(add(primitive.a, offset), scale),
                scalePoint(add(primitive.b, offset), scale),
                std::max(1.0f, static_cast<float>(scale)),
                primitive.color
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Rectangle)
        {
            drawPolygon(
                rectanglePoints(
                    add(primitive.a, offset),
                    primitive.size,
                    primitive.angle
                ),
                "outline",
                primitive.color,
                scale
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Text)
        {
            drawCenteredText(
                primitive.text,
                add(primitive.a, offset),
                primitive.fontSize,
                primitive.angle,
                primitive.color,
                scale
            );

            return;
        }

        const Vector2 center =
            add(primitive.a, offset);

        if (primitive.shapeType == "triangle")
        {
            const Vector2 top =
                rotateAround(
                    Vector2{ center.x, center.y - primitive.size.y / 2.0f },
                    center,
                    primitive.angle
                );

            const Vector2 left =
                rotateAround(
                    Vector2{ center.x - primitive.size.x / 2.0f, center.y + primitive.size.y / 2.0f },
                    center,
                    primitive.angle
                );

            const Vector2 right =
                rotateAround(
                    Vector2{ center.x + primitive.size.x / 2.0f, center.y + primitive.size.y / 2.0f },
                    center,
                    primitive.angle
                );

            if (isOutlineMode(primitive.shapeMode))
            {
                DrawLineEx(scalePoint(top, scale), scalePoint(left, scale), std::max(1.0f, static_cast<float>(scale)), primitive.color);
                DrawLineEx(scalePoint(left, scale), scalePoint(right, scale), std::max(1.0f, static_cast<float>(scale)), primitive.color);
                DrawLineEx(scalePoint(right, scale), scalePoint(top, scale), std::max(1.0f, static_cast<float>(scale)), primitive.color);
            }
            else
            {
                DrawTriangle(
                    scalePoint(top, scale),
                    scalePoint(left, scale),
                    scalePoint(right, scale),
                    primitive.color
                );
            }

            return;
        }

        if (primitive.shapeType == "rectangle")
        {
            drawPolygon(
                rectanglePoints(center, primitive.size, primitive.angle),
                primitive.shapeMode,
                primitive.color,
                scale
            );

            return;
        }

        if (primitive.shapeType == "circle")
        {
            float radius =
                primitive.radius;

            if (radius <= 0.0f)
            {
                radius =
                    std::max(primitive.size.x, primitive.size.y) / 2.0f;
            }

            if (isOutlineMode(primitive.shapeMode))
            {
                DrawCircleLines(
                    static_cast<int>(std::round(center.x * scale)),
                    static_cast<int>(std::round(center.y * scale)),
                    radius * scale,
                    primitive.color
                );
            }
            else
            {
                DrawCircle(
                    static_cast<int>(std::round(center.x * scale)),
                    static_cast<int>(std::round(center.y * scale)),
                    radius * scale,
                    primitive.color
                );
            }

            return;
        }

        if (primitive.shapeType == "polygon")
        {
            std::vector<Vector2> points;
            points.reserve(primitive.points.size());

            for (Vector2 point : primitive.points)
            {
                points.push_back(
                    rotateAround(
                        Vector2{ center.x + point.x, center.y + point.y },
                        center,
                        primitive.angle
                    )
                );
            }

            drawPolygon(
                points,
                primitive.shapeMode,
                primitive.color,
                scale
            );

            return;
        }

        if (primitive.shapeType == "line")
        {
            const float thickness =
                std::max(1.0f, primitive.size.x * scale);

            const Vector2 start =
                scalePoint(
                    rotateAround(
                        Vector2{ center.x, center.y - primitive.size.y / 2.0f },
                        center,
                        primitive.angle
                    ),
                    scale
                );

            const Vector2 end =
                scalePoint(
                    rotateAround(
                        Vector2{ center.x, center.y + primitive.size.y / 2.0f },
                        center,
                        primitive.angle
                    ),
                    scale
                );

            DrawLineEx(start, end, thickness, primitive.color);
            return;
        }

        if (primitive.shapeType == "text")
        {
            drawShapeText(
                primitive,
                center,
                scale
            );

            return;
        }

        const std::string mode =
            primitive.shapeMode;

        const Vector2 size =
            primitive.size;

        if (isOutlineMode(mode))
        {
            drawPolygon(
                rectanglePoints(center, size, 0.0f),
                "outline",
                primitive.color,
                scale
            );
        }
        else
        {
            const Vector2 topLeft =
                scalePoint(
                    Vector2{
                        center.x - size.x / 2.0f,
                        center.y - size.y / 2.0f
                    },
                    scale
                );

            DrawRectangle(
                static_cast<int>(std::round(topLeft.x)),
                static_cast<int>(std::round(topLeft.y)),
                static_cast<int>(std::round(size.x * scale)),
                static_cast<int>(std::round(size.y * scale)),
                primitive.color
            );
        }
    }
}

Vector2 DrawingRenderer::logicalToPhysical(
    Vector2 point,
    int scale
)
{
    return scalePoint(point, scale);
}

void DrawingRenderer::render(
    const std::vector<VisualPrimitive>& primitives,
    const std::vector<RuntimeObject>& objects,
    int scale,
    float screenWidth,
    float screenHeight
)
{
    for (const VisualPrimitive& primitive : primitives)
    {
        const std::vector<Vector2> offsets =
            presentationOffsets(
                primitive,
                objects,
                screenWidth,
                screenHeight
            );

        for (Vector2 offset : offsets)
        {
            drawPrimitiveAt(
                primitive,
                offset,
                scale
            );
        }
    }
}

