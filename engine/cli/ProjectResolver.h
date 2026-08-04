#pragma once

#include "CliExitCode.h"
#include "../diagnostics/Diagnostics.h"

#include <filesystem>

struct ProjectResolutionResult
{
    bool success = false;
    std::filesystem::path manifestPath;
    Diagnostics diagnostics;
    CliExitCode exitCode = CliExitCode::ProjectResolutionError;
};

class ProjectResolver
{
public:
    ProjectResolutionResult resolve(const std::filesystem::path& target) const;
};
