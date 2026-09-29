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
      size(size)
{
    this->hasOrigin = false;
    this->hasSize = false;
    this->hasVisualColor = false;
    this->originalHasVisualColor = false;
    this->visualColor = color;
    this->originalVisualColor = color;
    this->originalOffset = Vector2{ 0.0f, 0.0f };
    this->speed = 0.0f;
    this->angle = 0.0f;
    this->originSpeed = speed;
    this->group = "";
    this->attached = false;
    this->depth = 0;
    this->attachFollowX = false;
    this->attachFollowY = false;
    this->attachFollowAngle = false;
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
    const WorldExtent& worldExtent
)
{
    if (boundsMode != "wrap")
    {
        return;
    }

    const float minX =
        worldExtent.x;

    const float maxX =
        worldExtent.x + worldExtent.width;

    const float minY =
        worldExtent.y;

    const float maxY =
        worldExtent.y + worldExtent.height;

    if (position.x < minX)
    {
        position.x = maxX;
    }
    else if (position.x > maxX)
    {
        position.x = minX;
    }

    if (position.y < minY)
    {
        position.y = maxY;
    }
    else if (position.y > maxY)
    {
        position.y = minY;
    }
}
