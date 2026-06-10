#pragma once

#include <string>

struct FlxContext
{
    std::string name;
    std::string version;
    std::string engineVersion;
    std::string notes;

    std::string rootDirectory;
    std::string projectPath;
    std::string root;

    std::string screenTitle;
    int screenWidth = 320;
    int screenHeight = 180;
    int screenScale = 3;

    bool debugCollisions = false;
};
