#pragma once

#include "../diagnostics/Diagnostics.h"

#include <string>

struct ProjectMetadata
{
    std::string name;
    std::string version;
    std::string notes;
};

struct ProjectManifest
{
    ProjectMetadata metadata;
    std::string title;
    std::string engineRequirement;
    std::string path = ".";
    std::string root;
    std::string machine;
    std::string inputMapping;
};

struct ProjectManifestResult
{
    bool success = false;
    ProjectManifest manifest;
    Diagnostics diagnostics;
};
