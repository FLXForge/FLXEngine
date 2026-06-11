#include "FlxContextBuilder.h"
#include "../debug/Logger.h"

#include <fstream>
#include <filesystem>

FlxContext FlxContextBuilder::build(const std::string& path)
{
    FlxContext context;
    context.rootDirectory = getDirectory(path);
    context.projectPath = context.rootDirectory;

    std::ifstream file(path);

    if (!file.is_open())
    {
        Logger::error("project", "The project file could not be opened");
        return context;
    }

    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const size_t separator = line.find('=');

        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key =
            trim(line.substr(0, separator));

        const std::string value =
            trim(line.substr(separator + 1));

        assign(context, key, value);
    }

    if (!context.root.empty() && context.screenTitle.empty())
    {
        context.screenTitle = context.name;
    }

    return context;
}

void FlxContextBuilder::assign(
    FlxContext& context,
    const std::string& key,
    const std::string& value
)
{
    if (key == "name")
    {
        context.name = value;
    }
    else if (key == "version")
    {
        context.version = value;
    }
    else if (key == "engine")
    {
        context.engineVersion = value;
    }
    else if (key == "path")
    {
        context.projectPath =
            joinPath(context.rootDirectory, value);
    }
    else if (key == "root")
    {
        context.root = value;
    }
    else if (key == "title")
    {
        context.screenTitle = value;
    }
    else if (key == "screen.width")
    {
        context.screenWidth = std::stoi(value);
    }
    else if (key == "screen.height")
    {
        context.screenHeight = std::stoi(value);
    }
    else if (key == "screen.scale")
    {
        context.screenScale = std::stoi(value);
    }
    else if (key == "debug.collisions")
    {
        context.debugCollisions = parseBool(value);
    }
    else if (key == "debug.logs")
    {
        context.debugLogs = parseBool(value);
    }
    else if (key == "notes")
    {
        context.notes = value;
    }
}

std::string FlxContextBuilder::trim(const std::string& value)
{
    const size_t start = value.find_first_not_of(" \t\r\n");

    if (start == std::string::npos)
    {
        return "";
    }

    const size_t end = value.find_last_not_of(" \t\r\n");

    return value.substr(start, end - start + 1);
}

std::string FlxContextBuilder::getDirectory(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");

    if (slash == std::string::npos)
    {
        return ".";
    }

    return path.substr(0, slash);
}

std::string FlxContextBuilder::joinPath(
    const std::string& basePath,
    const std::string& childPath
)
{
    if (childPath.empty() || childPath == "./")
    {
        return basePath;
    }

    return (std::filesystem::path(basePath) / childPath).generic_string();
}

bool FlxContextBuilder::parseBool(const std::string& value)
{
    return value == "true" || value == "1" || value == "yes";
}
