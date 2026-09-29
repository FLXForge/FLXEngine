#pragma once

#include "MachineDefinition.h"
#include "../diagnostics/Diagnostics.h"

#include <string>

class MachineLoader
{
public:
    static MachineDefinition defaultMachine();
    static MachineDefinition load(const std::string& path);
    static MachineDefinition load(
        const std::string& path,
        Diagnostics& diagnostics
    );
};
