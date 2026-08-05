#pragma once

#include "../machine/MachineDefinition.h"

#include <string>

struct FlxContext
{
    std::string name;
    std::string version;
    std::string engineRequirement;
    std::string notes;
    std::string inputMappingSourceName;
    std::string inputMappingContent;
    std::string title;

    MachineDefinition machine;
};
