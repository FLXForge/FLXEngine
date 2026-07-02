#pragma once

#include "MachineDefinition.h"

#include <string>

class MachineLoader
{
public:
    static MachineDefinition defaultMachine();
    static MachineDefinition load(const std::string& path);
};
