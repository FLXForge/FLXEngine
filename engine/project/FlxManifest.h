#pragma once

#include <string>

struct FlxManifest
{
    std::string name;
    std::string version;
    std::string flxVersion;
    std::string path;
    std::string main;
    std::string notes;
    std::string rootDirectory;
};