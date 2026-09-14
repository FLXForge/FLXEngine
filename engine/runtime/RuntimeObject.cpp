#include "RuntimeObject.h"

RuntimeObject::RuntimeObject(
    const std::string& name,
    Vector2 origin,
    Vector2 size,
    Color color
)
    : name(name),
      runtimeId(name),
      definitionId(""),
      parentId(""),
      sourcePath(""),
      visible(true),
      alive(true),
      deadCalled(false),
      origin(origin),
      position(origin),
      previousPosition(origin),
      size(size),
      color(color)
{
    this->hasOrigin = false;
    this->originalOffset = Vector2{ 0.0f, 0.0f };
    this->shapeMode = "fill";
    this->radius = 0.0f;
    this->speed = 0.0f;
    this->angle = 0.0f;
    this->originSpeed = speed;
    this->group = "";
    this->attached = false;
    this->depth = 0;
    this->attachFollowX = false;
    this->attachFollowY = false;
    this->attachFollowAngle = false;
    this->shapeType = "block";
    this->textContent = "";
    this->rotationSpeed = 0.0f;
    this->acceleration = 0.0f;
    this->maxSpeed = 0.0f;
    this->inertia = 0.0f;
    this->velocity = Vector2{ 0.0f, 0.0f };
    this->angularVelocity = 0.0f;
    this->motionCommanded = false;
    this->rotationCommanded = false;
    this->boundsMode = "none";
    this->boundsOverflow = false;
    this->component = false;
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
