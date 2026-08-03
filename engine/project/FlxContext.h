#pragma once

#include "../machine/MachineDefinition.h"

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
    std::string machinePath;
    std::string inputMappingPath;
    std::string inputMappingSourceName;
    std::string inputMappingContent;

    std::string screenTitle;
    int screenWidth = 640;
    int screenHeight = 480;
    int screenScale = 1;
    bool screenWidthOverride = false;
    bool screenHeightOverride = false;
    bool screenScaleOverride = false;

    bool debugCollisions = false;
    bool debugLogs = false;
    bool debugConsole = false;

    std::string windowMode = "window";

    MachineDefinition machine;
};
