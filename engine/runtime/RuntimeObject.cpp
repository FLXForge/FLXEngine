#include "RuntimeObject.h"

#include <cmath>

static Vector2 rotatePoint(
    Vector2 point,
    Vector2 center,
    float angleDegrees
)
{
    const float radians = angleDegrees * DEG2RAD;

    const float translatedX = point.x - center.x;
    const float translatedY = point.y - center.y;

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

RuntimeObject::RuntimeObject(
    const std::string& name,
    Vector2 origin,
    Vector2 size,
    Color color
)
{
    this->name = name;
    this->origin = origin;
    this->position = origin;
    this->size = size;
    this->color = color;
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

void RuntimeObject::drawAt(Vector2 drawPosition, int scale) const
{

    if (shapeType == "triangle")
    {
        const float x = drawPosition.x * scale;
        const float y = drawPosition.y * scale;

        const float width = size.x * scale;
        const float height = size.y * scale;

        Vector2 center = { x, y };

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

        top = rotatePoint(top, center, angle);
        left = rotatePoint(left, center, angle);
        right = rotatePoint(right, center, angle);

        DrawLineV(top, left, color);
        DrawLineV(left, right, color);
        DrawLineV(right, top, color);
    }
    else if (shapeType == "rectangle")
    {
        const float x = drawPosition.x * scale;
        const float y = drawPosition.y * scale;

        const float width = size.x * scale;
        const float height = size.y * scale;

        Rectangle rect = {
            x,
            y,
            width,
            height
        };

        Vector2 origin = {
            width / 2.0f,
            height / 2.0f
        };

        DrawRectanglePro(
            rect,
            origin,
            angle,
            color
        );
    }
    else if (shapeType == "line")
    {
        const float x = drawPosition.x * scale;
        const float y = drawPosition.y * scale;

        const float thickness = size.x * scale;
        const float length = size.y * scale;

        const float radians =
            angle * DEG2RAD;

        Vector2 start = {
            x,
            y - length / 2.0f
        };

        Vector2 end = {
            x,
            y + length / 2.0f
        };

        start = rotatePoint(start, Vector2{ x, y }, angle);
        end = rotatePoint(end, Vector2{ x, y }, angle);

        DrawLineEx(
            start,
            end,
            thickness,
            color
        );
    }
    else
    {
        DrawRectangle(
            static_cast<int>(position.x * scale),
            static_cast<int>(position.y * scale),
            static_cast<int>(size.x * scale),
            static_cast<int>(size.y * scale),
            color
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