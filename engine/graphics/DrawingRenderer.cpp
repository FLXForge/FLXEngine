#include "DrawingRenderer.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    struct RenderTransform
    {
        WorldExtent world;
        float rasterWidth = 0.0f;
        float rasterHeight = 0.0f;

        float scaleX() const
        {
            return rasterWidth / world.width;
        }

        float scaleY() const
        {
            return rasterHeight / world.height;
        }

        float lineWidth() const
        {
            return std::max(
                1.0f,
                std::min(
                    std::abs(scaleX()),
                    std::abs(scaleY())
                )
            );
        }
    };

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

    Vector2 transformPoint(
        Vector2 point,
        const RenderTransform& transform
    )
    {
        return Vector2{
            (point.x - transform.world.x) * transform.scaleX(),
            (point.y - transform.world.y) * transform.scaleY()
        };
    }

    Vector2 transformSize(
        Vector2 size,
        const RenderTransform& transform
    )
    {
        return Vector2{
            size.x * transform.scaleX(),
            size.y * transform.scaleY()
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

    std::vector<Vector2> ellipsePoints(
        Vector2 center,
        Vector2 size,
        float angle
    )
    {
        constexpr int SegmentCount = 48;

        const float radiusX =
            size.x / 2.0f;
        const float radiusY =
            size.y / 2.0f;

        std::vector<Vector2> points;
        points.reserve(SegmentCount);

        for (int i = 0; i < SegmentCount; ++i)
        {
            const float theta =
                (static_cast<float>(i) / static_cast<float>(SegmentCount)) *
                2.0f *
                PI;

            points.push_back(
                rotateAround(
                    Vector2{
                        center.x + std::cos(theta) * radiusX,
                        center.y + std::sin(theta) * radiusY
                    },
                    center,
                    angle
                )
            );
        }

        return points;
    }

    std::vector<Vector2> presentationOffsets(
        const VisualPrimitive& primitive,
        const WorldExtent& world
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

        const float minX =
            world.x;
        const float maxX =
            world.x + world.width;
        const float minY =
            world.y;
        const float maxY =
            world.y + world.height;

        const bool overflowLeft =
            primitive.presentationPosition.x - radius < minX;

        const bool overflowRight =
            primitive.presentationPosition.x + radius > maxX;

        const bool overflowTop =
            primitive.presentationPosition.y - radius < minY;

        const bool overflowBottom =
            primitive.presentationPosition.y + radius > maxY;

        if (overflowLeft)
        {
            offsets.push_back(Vector2{ world.width, 0.0f });
        }

        if (overflowRight)
        {
            offsets.push_back(Vector2{ -world.width, 0.0f });
        }

        if (overflowTop)
        {
            offsets.push_back(Vector2{ 0.0f, world.height });
        }

        if (overflowBottom)
        {
            offsets.push_back(Vector2{ 0.0f, -world.height });
        }

        if (overflowLeft && overflowTop)
        {
            offsets.push_back(Vector2{ world.width, world.height });
        }

        if (overflowLeft && overflowBottom)
        {
            offsets.push_back(Vector2{ world.width, -world.height });
        }

        if (overflowRight && overflowTop)
        {
            offsets.push_back(Vector2{ -world.width, world.height });
        }

        if (overflowRight && overflowBottom)
        {
            offsets.push_back(Vector2{ -world.width, -world.height });
        }

        return offsets;
    }

    void drawPolygon(
        std::vector<Vector2> points,
        const std::string& mode,
        Color color,
        const RenderTransform& transform
    )
    {
        if (points.size() < 2)
        {
            return;
        }

        for (Vector2& point : points)
        {
            point =
                transformPoint(point, transform);
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
                transform.lineWidth(),
                color
            );
        }
    }

    void drawCenteredText(
        const std::string& text,
        Vector2 center,
        int fontSize,
        float angle,
        Color color,
        const RenderTransform& transform
    )
    {
        if (text.empty())
        {
            return;
        }

        const Font font =
            GetFontDefault();

        constexpr float spacing = 1.0f;

        const float textScale =
            std::max(
                1.0f,
                std::min(
                    std::abs(transform.scaleX()),
                    std::abs(transform.scaleY())
                )
            );

        const float scaledFontSize =
            static_cast<float>(fontSize) * textScale;

        std::vector<std::string> lines;
        size_t start = 0;

        while (start <= text.size())
        {
            const size_t end =
                text.find('\n', start);

            if (end == std::string::npos)
            {
                lines.push_back(text.substr(start));
                break;
            }

            lines.push_back(text.substr(start, end - start));
            start = end + 1;
        }

        const Vector2 lineMetrics =
            MeasureTextEx(
                font,
                "A",
                scaledFontSize,
                spacing * textScale
            );

        const float lineHeight =
            std::max(
                scaledFontSize,
                lineMetrics.y
            ) +
            spacing * textScale;

        const float totalHeight =
            lineHeight * static_cast<float>(lines.size());

        const Vector2 physicalCenter =
            transformPoint(center, transform);

        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (lines[i].empty())
            {
                continue;
            }

            const Vector2 measured =
                MeasureTextEx(
                    font,
                    lines[i].c_str(),
                    scaledFontSize,
                    spacing * textScale
                );

            const Vector2 localLineCenter = {
                0.0f,
                -totalHeight / 2.0f +
                    lineHeight * (static_cast<float>(i) + 0.5f)
            };

            const Vector2 lineCenter =
                rotateAround(
                    Vector2{
                        physicalCenter.x + localLineCenter.x,
                        physicalCenter.y + localLineCenter.y
                    },
                    physicalCenter,
                    angle
                );

            DrawTextPro(
                font,
                lines[i].c_str(),
                lineCenter,
                Vector2{ measured.x / 2.0f, lineHeight / 2.0f },
                angle,
                scaledFontSize,
                spacing * textScale,
                color
            );
        }
    }

    void drawPath(
        const std::vector<Vector2>& points,
        bool close,
        Color color,
        const RenderTransform& transform
    )
    {
        if (points.empty())
        {
            return;
        }

        if (points.size() == 1)
        {
            const Vector2 point =
                transformPoint(points.front(), transform);

            DrawRectangle(
                static_cast<int>(std::round(point.x)),
                static_cast<int>(std::round(point.y)),
                static_cast<int>(std::ceil(transform.lineWidth())),
                static_cast<int>(std::ceil(transform.lineWidth())),
                color
            );

            return;
        }

        const size_t limit =
            close ? points.size() : points.size() - 1;

        for (size_t i = 0; i < limit; ++i)
        {
            DrawLineEx(
                transformPoint(points[i], transform),
                transformPoint(points[(i + 1) % points.size()], transform),
                transform.lineWidth(),
                color
            );
        }
    }

    void drawEvenOddFill(
        const std::vector<Vector2>& logicalPoints,
        Color color,
        const RenderTransform& transform
    )
    {
        if (logicalPoints.size() < 3)
        {
            drawPath(
                logicalPoints,
                false,
                color,
                transform
            );

            return;
        }

        std::vector<Vector2> points;
        points.reserve(logicalPoints.size());

        for (Vector2 point : logicalPoints)
        {
            points.push_back(transformPoint(point, transform));
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
            transform
        );
    }

    void drawPrimitiveAt(
        const VisualPrimitive& primitive,
        Vector2 offset,
        const RenderTransform& transform
    )
    {
        if (primitive.kind == VisualPrimitiveKind::Pixel)
        {
            const Vector2 point =
                transformPoint(add(primitive.a, offset), transform);

            const Vector2 size =
                transformSize(Vector2{ 1.0f, 1.0f }, transform);

            DrawRectangle(
                static_cast<int>(std::round(point.x)),
                static_cast<int>(std::round(point.y)),
                std::max(1, static_cast<int>(std::ceil(std::abs(size.x)))),
                std::max(1, static_cast<int>(std::ceil(std::abs(size.y)))),
                primitive.color
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Line)
        {
            DrawLineEx(
                transformPoint(add(primitive.a, offset), transform),
                transformPoint(add(primitive.b, offset), transform),
                transform.lineWidth(),
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
                transform
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
                transform
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
                DrawLineEx(transformPoint(top, transform), transformPoint(left, transform), transform.lineWidth(), primitive.color);
                DrawLineEx(transformPoint(left, transform), transformPoint(right, transform), transform.lineWidth(), primitive.color);
                DrawLineEx(transformPoint(right, transform), transformPoint(top, transform), transform.lineWidth(), primitive.color);
            }
            else
            {
                DrawTriangle(
                    transformPoint(top, transform),
                    transformPoint(left, transform),
                    transformPoint(right, transform),
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
                transform
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

            const std::vector<Vector2> points =
                ellipsePoints(
                    center,
                    primitive.size,
                    primitive.angle
                );

            if (isOutlineMode(primitive.primitiveMode))
            {
                drawPath(
                    points,
                    true,
                    primitive.color,
                    transform
                );
            }
            else
            {
                const Vector2 physicalCenter =
                    transformPoint(center, transform);

                for (size_t i = 0; i < points.size(); ++i)
                {
                    DrawTriangle(
                        physicalCenter,
                        transformPoint(points[(i + 1) % points.size()], transform),
                        transformPoint(points[i], transform),
                        primitive.color
                    );
                }
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
                    transform
                );
            }
            else
            {
                drawPath(
                    points,
                    primitive.geometryMode == "close",
                    primitive.color,
                    transform
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
                transform
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
    return Vector2{
        point.x * scale,
        point.y * scale
    };
}

Vector2 DrawingRenderer::logicalToPhysical(
    Vector2 point,
    const WorldExtent& worldExtent,
    float rasterWidth,
    float rasterHeight
)
{
    if (worldExtent.width <= 0.0f ||
        worldExtent.height <= 0.0f)
    {
        return Vector2{ 0.0f, 0.0f };
    }

    return Vector2{
        (point.x - worldExtent.x) * (rasterWidth / worldExtent.width),
        (point.y - worldExtent.y) * (rasterHeight / worldExtent.height)
    };
}

void DrawingRenderer::render(
    const std::vector<VisualPrimitive>& primitives,
    const std::vector<RuntimeObject>& objects,
    int scale,
    float screenWidth,
    float screenHeight
)
{
    render(
        primitives,
        objects,
        WorldExtent{
            0.0f,
            0.0f,
            screenWidth,
            screenHeight
        },
        screenWidth * static_cast<float>(scale),
        screenHeight * static_cast<float>(scale)
    );
}

void DrawingRenderer::render(
    const std::vector<VisualPrimitive>& primitives,
    const std::vector<RuntimeObject>& objects,
    const WorldExtent& worldExtent,
    float rasterWidth,
    float rasterHeight
)
{
    (void)objects;

    if (worldExtent.width <= 0.0f ||
        worldExtent.height <= 0.0f ||
        rasterWidth <= 0.0f ||
        rasterHeight <= 0.0f)
    {
        return;
    }

    const RenderTransform transform{
        worldExtent,
        rasterWidth,
        rasterHeight
    };

    for (const VisualPrimitive& primitive : primitives)
    {
        const std::vector<Vector2> offsets =
            presentationOffsets(
                primitive,
                worldExtent
            );

        for (Vector2 offset : offsets)
        {
            drawPrimitiveAt(
                primitive,
                offset,
                transform
            );
        }
    }
}
