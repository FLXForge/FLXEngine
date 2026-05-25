#include "RuntimeObject.h"

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
}

void RuntimeObject::draw(int scale) const
{
    if (!visible)
    {
        return;
    }

    DrawRectangle(
        static_cast<int>(position.x * scale),
        static_cast<int>(position.y * scale),
        static_cast<int>(size.x * scale),
        static_cast<int>(size.y * scale),
        color
    );
}