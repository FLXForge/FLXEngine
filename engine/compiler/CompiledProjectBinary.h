#pragma once

#include "CompiledProject.h"

#include <string>

struct CompiledProjectBinaryResult
{
    bool success = false;
    CompiledProject project;
    Diagnostics diagnostics;
};

class CompiledProjectWriter
{
public:
    static bool write(
        const std::string& path,
        const CompiledProject& project,
        Diagnostics& diagnostics
    );
};

class CompiledProjectReader
{
public:
    static CompiledProjectBinaryResult read(
        const std::string& path
    );
};
