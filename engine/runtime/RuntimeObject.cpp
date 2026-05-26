#include "RuntimeObject.h"

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
    this->shapeType = "rectangle";
    this->rotationSpeed = 0.0f;
    this->acceleration = 0.0f;
    this->maxSpeed = 0.0f;
    this->inertia = 1.0f;
    this->velocity = Vector2{ 0.0f, 0.0f };
}

void RuntimeObject::draw(int scale) const
{
    if (!visible)
    {
        return;
    }

    if (shapeType == "triangle")
    {
        const float x = position.x * scale;
        const float y = position.y * scale;

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