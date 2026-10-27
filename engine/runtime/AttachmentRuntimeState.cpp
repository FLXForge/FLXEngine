#include "AttachmentRuntimeState.h"

namespace AttachmentRuntimeState
{
    void setOffset(RuntimeObject& object, Vector2 offset)
    {
        object.attachmentOffset = offset;
    }

    Vector2 offsetFor(const RuntimeObject& object)
    {
        return object.attachmentOffset.value_or(object.originalOffset);
    }
}
