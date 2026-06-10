#pragma once

#include "../runtime/RuntimeObject.h"

#include <string>
#include <vector>

class JsonLoader
{
public:
    static std::vector<RuntimeObject> loadObjects(
        const std::string& path
    );
};
