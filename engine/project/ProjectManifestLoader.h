#pragma once

#include "ProjectManifest.h"

#include <string>

class ProjectManifestLoader
{
public:
    static ProjectManifestResult load(const std::string& path);

private:
    static std::string trim(const std::string& value);
};
