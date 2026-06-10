#pragma once

#include "ObjectDefinition.h"
#include "RuntimeObject.h"

#include <string>

class RuntimeObjectBuilder
{
public:
    static RuntimeObject build(
        const ObjectDefinition& definition,
        const std::string& runtimeId,
        const std::string& parentId
    );
};
