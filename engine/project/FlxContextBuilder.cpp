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

    Logger::setConsoleEnabled(context.debugConsole);
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
    if (!context.screenWidthOverride)
    {
        context.screenWidth =
            context.machine.video.screenWidth;
    }

    if (!context.screenHeightOverride)
    {
        context.screenHeight =
            context.machine.video.screenHeight;
    }

    if (!context.screenScaleOverride)
    {
        context.screenScale =
            context.machine.video.outputScale;
    }
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
        ", player buttons " +
        std::to_string(context.machine.input.playerButtons) +
        ", system buttons " +
        std::to_string(context.machine.input.systemButtons)
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

    Logger::debug(
        "window",
        "Window mode: " + context.windowMode
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
    else if (key == "input.mapping")
    {
        context.inputMappingPath =
            joinPath(context.rootDirectory, value);
    }
    else if (key == "title")
    {
        context.screenTitle = value;
    }
    else if (key == "screen.width")
    {
        context.screenWidth =
            std::stoi(value);
        context.screenWidthOverride = true;
    }
    else if (key == "screen.height")
    {
        context.screenHeight =
            std::stoi(value);
        context.screenHeightOverride = true;
    }
    else if (key == "screen.scale")
    {
        context.screenScale =
            std::stoi(value);
        context.screenScaleOverride = true;
    }
    else if (key == "debug.collisions")
    {
        context.debugCollisions =
            parseBoolProperty(key, value, context.debugCollisions);
    }
    else if (key == "debug.logs")
    {
        context.debugLogs =
            parseBoolProperty(key, value, context.debugLogs);
    }
    else if (key == "debug.console")
    {
        context.debugConsole =
            parseBoolProperty(key, value, context.debugConsole);
    }
    else if (key == "window.mode")
    {
        if (value == "window" || value == "fullscreen")
        {
            context.windowMode = value;
        }
        else
        {
            Logger::warning(
                "project",
                "Invalid value for window.mode: '" + value +
                "'. Using window"
            );

            context.windowMode = "window";
        }
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

bool FlxContextBuilder::parseBoolProperty(
    const std::string& key,
    const std::string& value,
    bool defaultValue
)
{
    if (value == "true")
    {
        return true;
    }

    if (value == "false")
    {
        return false;
    }

    Logger::warning(
        "project",
        "Invalid boolean value for " + key + ": '" + value +
        "'. Using " + (defaultValue ? "true" : "false")
    );

    return defaultValue;
}
