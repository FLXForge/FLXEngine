#include "RuntimeObject.h"

#include <algorithm>
#include <cmath>

namespace
{
    Vector2 rotatePoint(
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

        const float rotatedX =
            translatedX * cosf(radians) -
            translatedY * sinf(radians);

        const float rotatedY =
            translatedX * sinf(radians) +
            translatedY * cosf(radians);

        return Vector2{
            center.x + rotatedX,
            center.y + rotatedY
        };
    }

    bool isOutlineMode(
        const std::string& mode
    )
    {
        return mode == "outline";
    }

    Vector2 toScreenPoint(
        Vector2 center,
        Vector2 local,
        int scale,
        float angle
    )
    {
        Vector2 point = {
            center.x + local.x * scale,
            center.y + local.y * scale
        };

        return rotatePoint(
            point,
            center,
            angle
        );
    }

    std::vector<Vector2> buildRectanglePoints(
        float width,
        float height
    )
    {
        return {
            Vector2{ -width / 2.0f, -height / 2.0f },
            Vector2{  width / 2.0f, -height / 2.0f },
            Vector2{  width / 2.0f,  height / 2.0f },
            Vector2{ -width / 2.0f,  height / 2.0f }
        };
    }

    void drawPolygonPoints(
        const std::vector<Vector2>& transformedPoints,
        const std::string& shapeMode,
        Color color,
        int scale
    )
    {
        if (transformedPoints.size() < 2)
        {
            return;
        }

        if (!isOutlineMode(shapeMode) && transformedPoints.size() >= 3)
        {
            for (size_t i = 1; i + 1 < transformedPoints.size(); ++i)
            {
                DrawTriangle(
                    transformedPoints[0],
                    transformedPoints[i + 1],
                    transformedPoints[i],
                    color
                );
            }

            return;
        }

        for (size_t i = 0; i < transformedPoints.size(); ++i)
        {
            const Vector2 current =
                transformedPoints[i];

            const Vector2 next =
                transformedPoints[
                    (i + 1) % transformedPoints.size()
                ];

            DrawLineEx(
                current,
                next,
                1.0f * scale,
                color
            );
        }
    }

    std::vector<Vector2> transformPoints(
        const std::vector<Vector2>& localPoints,
        Vector2 center,
        int scale,
        float angle
    )
    {
        std::vector<Vector2> transformed;

        transformed.reserve(localPoints.size());

        for (const auto& localPoint : localPoints)
        {
            transformed.push_back(
                toScreenPoint(
                    center,
                    localPoint,
                    scale,
                    angle
                )
            );
        }

        return transformed;
    }
}

RuntimeObject::RuntimeObject(
    const std::string& name,
    Vector2 origin,
    Vector2 size,
    Color color
)
{
    this->name = name;
    this->runtimeId = name;
    this->sourcePath = "";
    this->origin = origin;
    this->hasOrigin = false;
    this->position = origin;
    this->size = size;
    this->color = color;
    this->shapeMode = "fill";
    this->radius = 0.0f;
    this->speed = 120.0f;
    this->angle = 0.0f;
    this->originSpeed = speed;
    this->group = "";
    this->visible = true;
    this->alive = true;
    this->deadCalled = false;
    this->shapeType = "block";
    this->rotationSpeed = 0.0f;
    this->acceleration = 0.0f;
    this->maxSpeed = 0.0f;
    this->inertia = 1.0f;
    this->velocity = Vector2{ 0.0f, 0.0f };
    this->boundsMode = "none";
    this->boundsOverflow = false;
    this->collisionType = "none";
    this->collisionRadius = 0.0f;
}

void RuntimeObject::draw(
    int scale,
    float screenWidth,
    float screenHeight
) const
{
    if (!visible)
    {
        return;
    }

    drawAt(position, scale);

    if (boundsMode != "wrap" || !boundsOverflow)
    {
        return;
    }

    const float radius =
        std::sqrt(
            size.x * size.x +
            size.y * size.y
        ) / 2.0f;

    const bool overflowLeft =
        position.x - radius < 0.0f;

    const bool overflowRight =
        position.x + radius > screenWidth;

    const bool overflowTop =
        position.y - radius < 0.0f;

    const bool overflowBottom =
        position.y + radius > screenHeight;

    if (overflowLeft)
    {
        drawAt(
            Vector2{ position.x + screenWidth, position.y },
            scale
        );
    }

    if (overflowRight)
    {
        drawAt(
            Vector2{ position.x - screenWidth, position.y },
            scale
        );
    }

    if (overflowTop)
    {
        drawAt(
            Vector2{ position.x, position.y + screenHeight },
            scale
        );
    }

    if (overflowBottom)
    {
        drawAt(
            Vector2{ position.x, position.y - screenHeight },
            scale
        );
    }

    // Esquinas: cuando toca dos bordes a la vez.
    if (overflowLeft && overflowTop)
    {
        drawAt(
            Vector2{ position.x + screenWidth, position.y + screenHeight },
            scale
        );
    }

    if (overflowLeft && overflowBottom)
    {
        drawAt(
            Vector2{ position.x + screenWidth, position.y - screenHeight },
            scale
        );
    }

    if (overflowRight && overflowTop)
    {
        drawAt(
            Vector2{ position.x - screenWidth, position.y + screenHeight },
            scale
        );
    }

    if (overflowRight && overflowBottom)
    {
        drawAt(
            Vector2{ position.x - screenWidth, position.y - screenHeight },
            scale
        );
    }
}

    void RuntimeObject::drawAt(
        Vector2 drawPosition,
        int scale
    ) const
    {
        const float x =
            drawPosition.x * scale;

        const float y =
            drawPosition.y * scale;

        if (shapeType == "triangle")
        {
            const float width =
                size.x * scale;

            const float height =
                size.y * scale;

            const Vector2 center = {
                x,
                y
            };

            Vector2 top = {
                x,
                y - height / 2.0f
            };

            Vector2 left = {
                x - width / 2.0f,
                y + height / 2.0f
            };

            Vector2 right = {
                x + width / 2.0f,
                y + height / 2.0f
            };

            top =
                rotatePoint(
                    top,
                    center,
                    angle
                );

            left =
                rotatePoint(
                    left,
                    center,
                    angle
                );

            right =
                rotatePoint(
                    right,
                    center,
                    angle
                );

            if (isOutlineMode(shapeMode))
            {
                DrawLineV(top, left, color);
                DrawLineV(left, right, color);
                DrawLineV(right, top, color);
            }
            else
            {
                DrawTriangle(
                    top,
                    left,
                    right,
                    color
                );
            }

            return;
        }

        if (shapeType == "rectangle")
        {
            const float width =
                size.x;

            const float height =
                size.y;

            const Vector2 center = {
                x,
                y
            };

            const std::vector<Vector2> localPoints =
                buildRectanglePoints(
                    width,
                    height
                );

            const std::vector<Vector2> transformedPoints =
                transformPoints(
                    localPoints,
                    center,
                    scale,
                    angle
                );

            drawPolygonPoints(
                transformedPoints,
                shapeMode,
                color,
                scale
            );

            return;
        }

        if (shapeType == "circle")
        {
            float drawRadius =
                radius;

            if (drawRadius <= 0.0f)
            {
                drawRadius =
                    std::max(
                        size.x,
                        size.y
                    ) / 2.0f;
            }

            drawRadius *= scale;

            if (isOutlineMode(shapeMode))
            {
                DrawCircleLines(
                    static_cast<int>(x),
                    static_cast<int>(y),
                    drawRadius,
                    color
                );
            }
            else
            {
                DrawCircle(
                    static_cast<int>(x),
                    static_cast<int>(y),
                    drawRadius,
                    color
                );
            }

            return;
        }

        if (shapeType == "polygon")
        {
            if (points.size() < 2)
            {
                return;
            }

            const Vector2 center = {
                x,
                y
            };

            std::vector<Vector2> transformedPoints;

            transformedPoints.reserve(
                points.size()
            );

            for (const auto& localPoint : points)
            {
                transformedPoints.push_back(
                    toScreenPoint(
                        center,
                        localPoint,
                        scale,
                        angle
                    )
                );
            }

            if (!isOutlineMode(shapeMode) && transformedPoints.size() >= 3)
            {
                for (size_t i = 1; i + 1 < transformedPoints.size(); ++i)
                {
                    DrawTriangle(
                        transformedPoints[0],
                        transformedPoints[i + 1],
                        transformedPoints[i],
                        color
                    );
                }
            }
            else
            {
                for (size_t i = 0; i < transformedPoints.size(); ++i)
                {
                    const Vector2 current =
                        transformedPoints[i];

                    const Vector2 next =
                        transformedPoints[
                            (i + 1) % transformedPoints.size()
                        ];

                    DrawLineEx(
                        current,
                        next,
                        1.0f * scale,
                        color
                    );
                }
            }

            return;
        }

        if (shapeType == "line")
        {
            const float thickness =
                size.x * scale;

            const float length =
                size.y * scale;

            Vector2 start = {
                x,
                y - length / 2.0f
            };

            Vector2 end = {
                x,
                y + length / 2.0f
            };

            start =
                rotatePoint(
                    start,
                    Vector2{ x, y },
                    angle
                );

            end =
                rotatePoint(
                    end,
                    Vector2{ x, y },
                    angle
                );

            DrawLineEx(
                start,
                end,
                thickness,
                color
            );

            return;
        }

        const float width =
            size.x * scale;

        const float height =
            size.y * scale;

        if (isOutlineMode(shapeMode))
        {
            DrawRectangleLines(
                static_cast<int>(drawPosition.x * scale),
                static_cast<int>(drawPosition.y * scale),
                static_cast<int>(width),
                static_cast<int>(height),
                color
            );
        }
        else
        {
            DrawRectangle(
                static_cast<int>(drawPosition.x * scale),
                static_cast<int>(drawPosition.y * scale),
                static_cast<int>(width),
                static_cast<int>(height),
                color
            );
        }
    }

void RuntimeObject::drawCollision(float scale) const
{
    if (collisionType == "none")
    {
        return;
    }

    if (collisionType == "circle")
    {
        Vector2 center =
            position;

        if (shapeType == "block")
        {
            center = Vector2{
                position.x + size.x / 2.0f,
                position.y + size.y / 2.0f
            };
        }

        float radius =
            collisionRadius;

        if (radius <= 0.0f)
        {
            radius =
                std::max(size.x, size.y) / 2.0f;
        }

        DrawCircleLines(
            static_cast<int>(center.x * scale),
            static_cast<int>(center.y * scale),
            radius * scale,
            GREEN
        );

        return;
    }

    if (collisionType == "box")
    {
        DrawRectangleLines(
            static_cast<int>(position.x * scale),
            static_cast<int>(position.y * scale),
            static_cast<int>(size.x * scale),
            static_cast<int>(size.y * scale),
            GREEN
        );
    }
}

void RuntimeObject::applyBounds(
    float screenWidth,
    float screenHeight
)
{
    if (boundsMode != "wrap")
    {
        return;
    }

    if (position.x < 0.0f)
    {
        position.x = screenWidth;
    }
    else if (position.x > screenWidth)
    {
        position.x = 0.0f;
    }

    if (position.y < 0.0f)
    {
        position.y = screenHeight;
    }
    else if (position.y > screenHeight)
    {
        position.y = 0.0f;
    }
}