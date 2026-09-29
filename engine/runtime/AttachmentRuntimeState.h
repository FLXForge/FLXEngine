#pragma once

#include "RuntimeObject.h"

#include <raylib.h>
#include <string>
#include <vector>

namespace AttachmentRuntimeState
{
    void clear();
    void registerObject(const RuntimeObject& object);
    void setOffset(
        const std::string& runtimeId,
        Vector2 offset
    );
    Vector2 offsetFor(const RuntimeObject& object);
    void pruneMissing(const std::vector<RuntimeObject>& objects);
}
