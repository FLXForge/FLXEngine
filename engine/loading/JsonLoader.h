#pragma once

#include "../project/GameConfig.h"
#include "../runtime/RuntimeObject.h"

#include <string>
#include <vector>

class JsonLoader
{
public:
    static std::vector<RuntimeObject> loadObjects(
        const std::string& path
    );
    static GameConfig loadGameConfig(const std::string& path);
};