#include "FlxContextBuilder.h"
#include "../debug/Logger.h"
#include "../machine/MachineLoader.h"

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

    Logger::setDebugEnabled(context.debugLogs);

    loadMachine(context);
    applyMachineScreenDefaults(context);
    logResolvedContext(context);

    return context;
}

void FlxContextBuilder::loadMachine(FlxContext& context)
{
    if (context.machinePath.empty())
    {
        context.machine =
            MachineLoader::defaultMachine();

        Logger::debug(
            "machine",
            "Using internal default machine"
        );

        return;
    }

    context.machine =
        MachineLoader::load(context.machinePath);
}

void FlxContextBuilder::applyMachineScreenDefaults(FlxContext& context)
{
    context.screenWidth =
        context.machine.video.screenWidth;

    context.screenHeight =
        context.machine.video.screenHeight;

    context.screenScale =
        context.machine.video.outputScale;
}

void FlxContextBuilder::logResolvedContext(const FlxContext& context)
{
    const std::string machineSource =
        context.machinePath.empty()
        ? "internal default"
        : context.machinePath;

    Logger::debug(
        "machine",
        "Machine: " + machineSource
    );

    Logger::debug(
        "machine",
        "Video screen: " +
        std::to_string(context.machine.video.screenWidth) +
        "x" +
        std::to_string(context.machine.video.screenHeight) +
        " scale " +
        std::to_string(context.machine.video.outputScale)
    );

    Logger::debug(
        "machine",
        "Audio voices: music " +
        std::to_string(context.machine.audio.voicesMusic) +
        ", sound " +
        std::to_string(context.machine.audio.voicesSound) +
        ", mode " +
        context.machine.audio.voicesMode +
        ", overflow " +
        context.machine.audio.voicesOverflow
    );

    Logger::debug(
        "machine",
        "Audio synthesis: " +
        context.machine.audio.synthesisModel +
        ", " +
        context.machine.audio.synthesisTexture +
        ", " +
        context.machine.audio.synthesisMovement +
        ", noise " +
        context.machine.audio.synthesisNoise
    );

    Logger::debug(
        "machine",
        "Audio fidelity: " +
        context.machine.audio.fidelityResolution +
        ", " +
        context.machine.audio.fidelityDynamics +
        ", " +
        context.machine.audio.fidelitySpace
    );

    Logger::debug(
        "machine",
        "Input capabilities: " +
        context.machine.input.direction +
        ", buttons " +
        std::to_string(context.machine.input.buttons)
    );

    Logger::debug(
        "machine",
        "Resolved screen: " +
        std::to_string(context.screenWidth) +
        "x" +
        std::to_string(context.screenHeight) +
        " scale " +
        std::to_string(context.screenScale)
    );
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
    else if (key == "machine")
    {
        context.machinePath =
            joinPath(context.rootDirectory, value);
    }
    else if (key == "title")
    {
        context.screenTitle = value;
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
