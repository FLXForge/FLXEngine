#include "DrawingRenderer.h"

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

        if (!primitive.presentationWrap || !primitive.presentationOverflow)
        {
            return offsets;
        }

        const float radius =
            std::sqrt(
                primitive.presentationSize.x * primitive.presentationSize.x +
                primitive.presentationSize.y * primitive.presentationSize.y
            ) / 2.0f;

        const bool overflowLeft =
            primitive.presentationPosition.x - radius < 0.0f;

        const bool overflowRight =
            primitive.presentationPosition.x + radius > screenWidth;

        const bool overflowTop =
            primitive.presentationPosition.y - radius < 0.0f;

        const bool overflowBottom =
            primitive.presentationPosition.y + radius > screenHeight;

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

    void drawPath(
        const std::vector<Vector2>& points,
        bool close,
        Color color,
        int scale
    )
    {
        if (points.empty())
        {
            return;
        }

        if (points.size() == 1)
        {
            const Vector2 point =
                scalePoint(points.front(), scale);

            DrawRectangle(
                static_cast<int>(std::round(point.x)),
                static_cast<int>(std::round(point.y)),
                std::max(1, scale),
                std::max(1, scale),
                color
            );

            return;
        }

        const size_t limit =
            close ? points.size() : points.size() - 1;

        for (size_t i = 0; i < limit; ++i)
        {
            DrawLineEx(
                scalePoint(points[i], scale),
                scalePoint(points[(i + 1) % points.size()], scale),
                std::max(1.0f, static_cast<float>(scale)),
                color
            );
        }
    }

    void drawEvenOddFill(
        const std::vector<Vector2>& logicalPoints,
        Color color,
        int scale
    )
    {
        if (logicalPoints.size() < 3)
        {
            drawPath(
                logicalPoints,
                false,
                color,
                scale
            );

            return;
        }

        std::vector<Vector2> points;
        points.reserve(logicalPoints.size());

        for (Vector2 point : logicalPoints)
        {
            points.push_back(scalePoint(point, scale));
        }

        float minY =
            points.front().y;
        float maxY =
            points.front().y;

        for (Vector2 point : points)
        {
            minY =
                std::min(minY, point.y);
            maxY =
                std::max(maxY, point.y);
        }

        const int firstY =
            static_cast<int>(std::ceil(minY));
        const int lastY =
            static_cast<int>(std::floor(maxY));

        for (int y = firstY; y <= lastY; ++y)
        {
            const float scanY =
                static_cast<float>(y) + 0.5f;

            std::vector<float> intersections;

            for (size_t i = 0; i < points.size(); ++i)
            {
                const Vector2 a =
                    points[i];
                const Vector2 b =
                    points[(i + 1) % points.size()];

                if (std::abs(a.y - b.y) <= 0.00001f)
                {
                    continue;
                }

                const float minEdgeY =
                    std::min(a.y, b.y);
                const float maxEdgeY =
                    std::max(a.y, b.y);

                if (scanY < minEdgeY || scanY >= maxEdgeY)
                {
                    continue;
                }

                const float t =
                    (scanY - a.y) / (b.y - a.y);

                intersections.push_back(
                    a.x + t * (b.x - a.x)
                );
            }

            std::sort(
                intersections.begin(),
                intersections.end()
            );

            for (size_t i = 0; i + 1 < intersections.size(); i += 2)
            {
                const int startX =
                    static_cast<int>(std::ceil(intersections[i]));
                const int endX =
                    static_cast<int>(std::floor(intersections[i + 1]));

                if (endX < startX)
                {
                    continue;
                }

                DrawLine(
                    startX,
                    y,
                    endX,
                    y,
                    color
                );
            }
        }

        drawPath(
            logicalPoints,
            true,
            color,
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

        if (primitive.primitive == "triangle")
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

            if (isOutlineMode(primitive.primitiveMode))
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

        if (primitive.primitive == "rectangle")
        {
            drawPolygon(
                rectanglePoints(center, primitive.size, primitive.angle),
                primitive.primitiveMode,
                primitive.color,
                scale
            );

            return;
        }

        if (primitive.primitive == "ellipse")
        {
            const float radiusX =
                primitive.size.x / 2.0f;
            const float radiusY =
                primitive.size.y / 2.0f;

            if (radiusX <= 0.0f || radiusY <= 0.0f)
            {
                return;
            }

            if (isOutlineMode(primitive.primitiveMode))
            {
                const float thickness =
                    std::max(1.0f, static_cast<float>(scale));
                const Vector2 physicalCenter =
                    scalePoint(center, scale);
                const float physicalRadiusX =
                    radiusX * scale;
                const float physicalRadiusY =
                    radiusY * scale;

                DrawEllipseLines(
                    static_cast<int>(std::round(physicalCenter.x)),
                    static_cast<int>(std::round(physicalCenter.y)),
                    physicalRadiusX,
                    physicalRadiusY,
                    primitive.color
                );

                if (thickness > 1.0f)
                {
                    DrawEllipseLines(
                        static_cast<int>(std::round(physicalCenter.x)),
                        static_cast<int>(std::round(physicalCenter.y)),
                        std::max(0.0f, physicalRadiusX - thickness),
                        std::max(0.0f, physicalRadiusY - thickness),
                        primitive.color
                    );
                }
            }
            else
            {
                DrawEllipse(
                    static_cast<int>(std::round(center.x * scale)),
                    static_cast<int>(std::round(center.y * scale)),
                    radiusX * scale,
                    radiusY * scale,
                    primitive.color
                );
            }

            return;
        }

        if (primitive.primitive == "geometry")
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

            if (primitive.geometryMode == "fill")
            {
                drawEvenOddFill(
                    points,
                    primitive.color,
                    scale
                );
            }
            else
            {
                drawPath(
                    points,
                    primitive.geometryMode == "close",
                    primitive.color,
                    scale
                );
            }

            return;
        }

        if (primitive.primitive == "text")
        {
            drawCenteredText(
                primitive.text,
                center,
                primitive.fontSize,
                primitive.angle,
                primitive.color,
                scale
            );

            return;
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
    (void)objects;

    for (const VisualPrimitive& primitive : primitives)
    {
        const std::vector<Vector2> offsets =
            presentationOffsets(
                primitive,
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

