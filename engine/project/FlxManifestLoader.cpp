#include "FlxManifestLoader.h"
#include "../debug/Logger.h"

#include <fstream>
#include <iostream>

FlxManifest FlxManifestLoader::load(const std::string& path)
{
    FlxManifest manifest;
    manifest.rootDirectory = getDirectory(path);

    std::ifstream file(path);

    if (!file.is_open())
    {
        Logger::error("project", "The manifesto could not be opened");
        return manifest;
    }

    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty())
        {
            continue;
        }

        if (line[0] == '#')
        {
            continue;
        }

        const size_t separator = line.find(':');

        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key =
            trim(line.substr(0, separator));

        const std::string value =
            trim(line.substr(separator + 1));

        if (key == "name")
        {
            manifest.name = value;
        }
        else if (key == "version")
        {
            manifest.version = value;
        }
        else if (key == "flxVersion")
        {
            manifest.flxVersion = value;
        }
        else if (key == "path")
        {
            manifest.path = value;
        }
        else if (key == "main")
        {
            manifest.main = value;
        }
        else if (key == "notes")
        {
            manifest.notes = value;
        }
    }

    return manifest;
}

std::string FlxManifestLoader::trim(const std::string& value)
{
    const size_t start = value.find_first_not_of(" \t\r\n");

    if (start == std::string::npos)
    {
        return "";
    }

    const size_t end = value.find_last_not_of(" \t\r\n");

    return value.substr(start, end - start + 1);
}

std::string FlxManifestLoader::getDirectory(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");

    if (slash == std::string::npos)
    {
        return ".";
    }

    return path.substr(0, slash);
}