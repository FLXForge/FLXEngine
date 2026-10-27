#pragma once

#include "RuntimeObject.h"

namespace AttachmentRuntimeState
{
    void setOffset(RuntimeObject& object, Vector2 offset);
    Vector2 offsetFor(const RuntimeObject& object);
}
