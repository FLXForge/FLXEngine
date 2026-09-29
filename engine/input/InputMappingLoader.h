#pragma once

#include "InputMapping.h"
#include "../diagnostics/Diagnostics.h"
#include "../machine/MachineDefinition.h"

#include <string>

struct InputMappingLoadResult
{
    bool success = false;
    Diagnostics diagnostics;
    InputMapping mapping;
    std::string sourceName;
    std::string content;
};

class InputMappingLoader
{
public:
    static std::string defaultMappingContent();

    static InputMappingLoadResult loadDefault(
        const InputChipDefinition& chip
    );

    static InputMappingLoadResult loadFile(
        const std::string& path,
        const InputChipDefinition& chip
    );

    static InputMappingLoadResult loadContent(
        const std::string& sourceName,
        const std::string& content,
        const InputChipDefinition& chip
    );
};
