#pragma once

#include "CompiledProject.h"

#include <cstdint>
#include <string>

struct CompiledProjectBinaryMetadata
{
    uint32_t formatVersion = 0;
    std::string producerVersion;
};

struct CompiledProjectBinaryResult
{
    bool success = false;
    CompiledProject project;
    CompiledProjectBinaryMetadata metadata;
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
