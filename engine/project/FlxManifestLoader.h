#pragma once

#include "FlxManifest.h"

#include <string>

class FlxManifestLoader
{
public:
    static FlxManifest load(const std::string& path);

private:
    static std::string trim(const std::string& value);
    static std::string getDirectory(const std::string& path);
};