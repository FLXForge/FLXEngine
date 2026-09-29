#include "AttachmentRuntimeState.h"

#include <unordered_map>

namespace
{
    std::unordered_map<std::string, Vector2> attachmentOffsets;
}

namespace AttachmentRuntimeState
{
    void clear()
    {
        attachmentOffsets.clear();
    }

    void registerObject(const RuntimeObject& object)
    {
        attachmentOffsets.emplace(
            object.runtimeId,
            object.originalOffset
        );
    }

    void setOffset(
        const std::string& runtimeId,
        Vector2 offset
    )
    {
        attachmentOffsets[runtimeId] =
            offset;
    }

    Vector2 offsetFor(const RuntimeObject& object)
    {
        const auto it =
            attachmentOffsets.find(object.runtimeId);

        if (it == attachmentOffsets.end())
        {
            return object.originalOffset;
        }

        return it->second;
    }

    void pruneMissing(const std::vector<RuntimeObject>& objects)
    {
        for (auto it = attachmentOffsets.begin();
            it != attachmentOffsets.end();)
        {
            const std::string runtimeId =
                it->first;

            bool found =
                false;

            for (const auto& object : objects)
            {
                if (object.alive &&
                    object.runtimeId == runtimeId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                it = attachmentOffsets.erase(it);
                continue;
            }

            ++it;
        }
    }
}
