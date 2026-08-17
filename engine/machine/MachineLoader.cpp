#include "MachineLoader.h"
#include "../debug/Logger.h"
#include "../tools/ColorParser.h"

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <unordered_set>

namespace
{
    std::string trim(const std::string& value)
    {
        const size_t start =
            value.find_first_not_of(" \t\r\n");

        if (start == std::string::npos)
        {
            return "";
        }

        const size_t end =
            value.find_last_not_of(" \t\r\n");

        return value.substr(start, end - start + 1);
    }

    std::string nodeToString(const YAML::Node& node)
    {
        if (!node || !node.IsDefined() || node.IsNull() || !node.IsScalar())
        {
            return "";
        }

        return trim(node.as<std::string>());
    }

    YAML::Node childNode(
        const YAML::Node& node,
        const std::initializer_list<std::string>& path
    )
    {
        YAML::Node current =
            YAML::Clone(node);

        for (const auto& key : path)
        {
            if (!current || !current.IsDefined() || current.IsNull() || !current.IsMap())
            {
                return {};
            }

            YAML::Node next;

            for (const auto& child : current)
            {
                if (
                    child.first.IsScalar() &&
                    child.first.as<std::string>() == key
                )
                {
                    next =
                        YAML::Clone(child.second);

                    break;
                }
            }

            current =
                next;

            if (!current || !current.IsDefined() || current.IsNull())
            {
                return {};
            }
        }

        return current;
    }

    bool hasNode(const YAML::Node& node)
    {
        return node && node.IsDefined() && !node.IsNull();
    }

    bool isHexDigit(char value)
    {
        return std::isxdigit(
            static_cast<unsigned char>(value)
        ) != 0;
    }

    bool isImplicitHexColor(
        const std::string& line,
        size_t valueStart,
        size_t& valueEnd
    )
    {
        if (valueStart >= line.size() || line[valueStart] != '#')
        {
            return false;
        }

        size_t cursor =
            valueStart + 1;

        while (cursor < line.size() && isHexDigit(line[cursor]))
        {
            ++cursor;
        }

        const size_t hexLength =
            cursor - valueStart - 1;

        if (hexLength != 3 && hexLength != 6)
        {
            return false;
        }

        size_t rest =
            cursor;

        while (rest < line.size() && std::isspace(static_cast<unsigned char>(line[rest])) != 0)
        {
            ++rest;
        }

        if (rest < line.size() && line[rest] != '#')
        {
            return false;
        }

        valueEnd =
            cursor;

        return true;
    }

    std::string normalizeYamlLine(const std::string& line)
    {
        const size_t separator =
            line.find(':');

        if (separator == std::string::npos)
        {
            return line;
        }

        size_t valueStart =
            separator + 1;

        while (valueStart < line.size() && std::isspace(static_cast<unsigned char>(line[valueStart])) != 0)
        {
            ++valueStart;
        }

        size_t valueEnd =
            valueStart;

        if (!isImplicitHexColor(line, valueStart, valueEnd))
        {
            return line;
        }

        return
            line.substr(0, valueStart) +
            "\"" +
            line.substr(valueStart, valueEnd - valueStart) +
            "\"" +
            line.substr(valueEnd);
    }

    std::string normalizeYamlText(const std::string& text)
    {
        std::stringstream input(text);
        std::stringstream output;
        std::string line;
        bool firstLine = true;

        while (std::getline(input, line))
        {
            if (!firstLine)
            {
                output << '\n';
            }

            output << normalizeYamlLine(line);
            firstLine = false;
        }

        return output.str();
    }

    bool nodeToBool(
        const YAML::Node& node,
        bool fallback,
        const std::string& fieldName
    )
    {
        if (!node || !node.IsDefined() || node.IsNull())
        {
            return fallback;
        }

        if (!node.IsScalar())
        {
            Logger::warning(
                "machine",
                "Invalid boolean field '" + fieldName +
                "', using " + std::string(fallback ? "true" : "false")
            );

            return fallback;
        }

        const std::string value =
            nodeToString(node);

        if (value == "true" || value == "1" || value == "yes")
        {
            return true;
        }

        if (value == "false" || value == "0" || value == "no")
        {
            return false;
        }

        Logger::warning(
            "machine",
            "Invalid boolean field '" + fieldName +
            "', using " + std::string(fallback ? "true" : "false")
        );

        return fallback;
    }

    int nodeToInt(
        const YAML::Node& node,
        int fallback,
        const std::string& fieldName,
        int minimum
    )
    {
        if (!node || !node.IsDefined() || node.IsNull())
        {
            return fallback;
        }

        if (!node.IsScalar())
        {
            Logger::warning(
                "machine",
                "Invalid numeric field '" + fieldName +
                "', using " + std::to_string(fallback)
            );

            return fallback;
        }

        try
        {
            const int parsedValue =
                node.as<int>();

            if (parsedValue < minimum)
            {
                Logger::warning(
                    "machine",
                    "Invalid numeric field '" + fieldName +
                    "', using " + std::to_string(fallback)
                );

                return fallback;
            }

            return parsedValue;
        }
        catch (const YAML::Exception&)
        {
            Logger::warning(
                "machine",
                "Invalid numeric field '" + fieldName +
                "', using " + std::to_string(fallback)
            );

            return fallback;
        }
    }

    std::string nodeToEnum(
        const YAML::Node& node,
        const std::string& fallback,
        const std::string& fieldName,
        const std::unordered_set<std::string>& validValues
    )
    {
        if (!node || !node.IsDefined() || node.IsNull())
        {
            return fallback;
        }

        if (!node.IsScalar())
        {
            Logger::warning(
                "machine",
                "Invalid enum field '" + fieldName +
                "', using '" + fallback + "'"
            );

            return fallback;
        }

        const std::string value =
            nodeToString(node);

        if (validValues.contains(value))
        {
            return value;
        }

        Logger::warning(
            "machine",
            "Invalid enum field '" + fieldName +
            "' value '" + value + "', using '" + fallback + "'"
        );

        return fallback;
    }

    std::string joinPath(
        const std::string& basePath,
        const std::string& childPath
    )
    {
        return (std::filesystem::path(basePath) / childPath).generic_string();
    }

    std::string directoryOf(const std::string& path)
    {
        return std::filesystem::path(path).parent_path().generic_string();
    }

    YAML::Node loadYamlNode(const std::string& path)
    {
        if (!std::filesystem::exists(path))
        {
            Logger::error(
                "machine",
                "Machine resource not found: " + path
            );

            return {};
        }

        try
        {
            std::ifstream file(path);

            if (!file.is_open())
            {
                Logger::error(
                    "machine",
                    "Machine resource could not be opened: " + path
                );

                return {};
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            return YAML::Load(
                normalizeYamlText(buffer.str())
            );
        }
        catch (const YAML::Exception& exception)
        {
            Logger::error(
                "machine",
                "Invalid YAML in " + path + ": " + exception.what()
            );

            return {};
        }
    }

    YAML::Node resolveChipNode(
        const YAML::Node& node,
        const std::string& machineDirectory,
        const std::string& chipName
    )
    {
        if (!node)
        {
            return {};
        }

        if (node.IsMap())
        {
            return YAML::Clone(node);
        }

        if (!node.IsScalar())
        {
            Logger::error(
                "machine",
                "Invalid " + chipName + " chip declaration"
            );

            return {};
        }

        const std::string chipPath =
            joinPath(
                machineDirectory,
                nodeToString(node)
            );

        YAML::Node chipFile =
            loadYamlNode(chipPath);

        if (!chipFile)
        {
            return {};
        }

        YAML::Node chipRoot =
            chipFile[chipName];

        if (!chipRoot || !chipRoot.IsMap())
        {
            Logger::error(
                "machine",
                "Chip file does not contain a valid '" +
                chipName + "' block: " + chipPath
            );

            return {};
        }

        return YAML::Clone(chipRoot);
    }

    void assignVideoChip(
        VideoChipDefinition& chip,
        const YAML::Node& node
    )
    {
        const YAML::Node screenNode =
            childNode(node, { "screen" });

        const YAML::Node colorNode =
            childNode(node, { "color" });

        const YAML::Node outputNode =
            childNode(node, { "output" });

        chip.screenWidth =
            nodeToInt(
                childNode(screenNode, { "width" }),
                chip.screenWidth,
                "video.screen.width",
                1
            );

        chip.screenHeight =
            nodeToInt(
                childNode(screenNode, { "height" }),
                chip.screenHeight,
                "video.screen.height",
                1
            );

        const std::string screenColor =
            nodeToString(
                childNode(screenNode, { "color" })
            );

        chip.clearColor =
            screenColor.empty()
            ? chip.clearColor
            : screenColor;

        chip.colorPalette.clear();
        chip.hasColorPalette = false;

        const YAML::Node paletteNode =
            childNode(colorNode, { "palette" });

        const YAML::Node levelsNode =
            childNode(colorNode, { "levels" });

        if (hasNode(paletteNode))
        {
            if (!paletteNode.IsSequence())
            {
                Logger::warning(
                    "machine",
                    "Invalid color.palette, using default color model"
                );
            }
            else
            {
                for (const auto& colorNode : paletteNode)
                {
                    if (!colorNode.IsScalar())
                    {
                        Logger::warning(
                            "machine",
                            "Invalid color.palette entry ignored"
                        );

                        continue;
                    }

                    chip.colorPalette.push_back(
                        ColorParser::parse(
                            nodeToString(colorNode),
                            WHITE
                        )
                    );
                }

                chip.hasColorPalette =
                    !chip.colorPalette.empty();
            }
        }

        chip.colorLevelsRed =
            nodeToInt(
                childNode(levelsNode, { "red" }),
                chip.colorLevelsRed,
                "video.color.levels.red",
                0
            );

        chip.colorLevelsGreen =
            nodeToInt(
                childNode(levelsNode, { "green" }),
                chip.colorLevelsGreen,
                "video.color.levels.green",
                0
            );

        chip.colorLevelsBlue =
            nodeToInt(
                childNode(levelsNode, { "blue" }),
                chip.colorLevelsBlue,
                "video.color.levels.blue",
                0
            );

        chip.hasColorLevels =
            hasNode(levelsNode) &&
            levelsNode.IsMap();

        const std::string toneBase =
            nodeToString(
                childNode(colorNode, { "tone", "base" })
            );

        chip.colorToneBase =
            toneBase.empty()
            ? chip.colorToneBase
            : toneBase;

        chip.colorToneLevels =
            nodeToInt(
                childNode(colorNode, { "tone", "levels" }),
                chip.colorToneLevels,
                "video.color.tone.levels",
                1
            );

        chip.hasColorTone =
            !chip.colorToneBase.empty() &&
            chip.colorToneLevels > 0;

        chip.colorAlpha =
            nodeToBool(
                childNode(colorNode, { "alpha" }),
                chip.colorAlpha,
                "video.color.alpha"
            );

        chip.planesEnabled =
            nodeToBool(
                childNode(node, { "planes", "enabled" }),
                chip.planesEnabled,
                "video.planes.enabled"
            );

        chip.objectsSprites =
            nodeToBool(
                childNode(node, { "objects", "sprites" }),
                chip.objectsSprites,
                "video.objects.sprites"
            );

        chip.outputScale =
            nodeToInt(
                childNode(outputNode, { "scale" }),
                chip.outputScale,
                "video.output.scale",
                1
            );

        chip.smoothing =
            nodeToBool(
                childNode(outputNode, { "smoothing" }),
                chip.smoothing,
                "video.output.smoothing"
            );
    }

    void assignAudioChip(
        AudioChipDefinition& chip,
        const YAML::Node& node
    )
    {
        const YAML::Node voicesNode =
            childNode(node, { "voices" });

        const YAML::Node synthesisNode =
            childNode(node, { "synthesis" });

        const YAML::Node fidelityNode =
            childNode(node, { "fidelity" });

        const YAML::Node resourcesNode =
            childNode(node, { "resources" });

        const YAML::Node fileAudioNode =
            childNode(node, { "fileAudio" });

        chip.voicesMusic =
            nodeToInt(
                childNode(voicesNode, { "music" }),
                chip.voicesMusic,
                "audio.voices.music",
                0
            );

        chip.voicesSound =
            nodeToInt(
                childNode(voicesNode, { "sound" }),
                chip.voicesSound,
                "audio.voices.sound",
                0
            );

        chip.voicesMode =
            nodeToEnum(
                childNode(voicesNode, { "mode" }),
                chip.voicesMode,
                "audio.voices.mode",
                { "shared", "preferred", "reserved" }
            );

        chip.voicesOverflow =
            nodeToEnum(
                childNode(voicesNode, { "overflow" }),
                chip.voicesOverflow,
                "audio.voices.overflow",
                {
                    "ignore",
                    "replace_oldest",
                    "replace_newest",
                    "replace_lowest_priority",
                    "steal_from_music"
                }
            );

        chip.synthesisModel =
            nodeToEnum(
                childNode(synthesisNode, { "model" }),
                chip.synthesisModel,
                "audio.synthesis.model",
                { "buzzer", "pulse", "wave", "mixed", "fm", "sample", "open" }
            );

        chip.synthesisTexture =
            nodeToEnum(
                childNode(synthesisNode, { "texture" }),
                chip.synthesisTexture,
                "audio.synthesis.texture",
                { "raw", "coarse", "clean", "rough", "rich" }
            );

        chip.synthesisMovement =
            nodeToEnum(
                childNode(synthesisNode, { "movement" }),
                chip.synthesisMovement,
                "audio.synthesis.movement",
                { "none", "simple", "dynamic", "expressive" }
            );

        chip.synthesisNoise =
            nodeToEnum(
                childNode(synthesisNode, { "noise" }),
                chip.synthesisNoise,
                "audio.synthesis.noise",
                { "none", "simple", "rich" }
            );

        chip.fidelityResolution =
            nodeToEnum(
                childNode(fidelityNode, { "resolution" }),
                chip.fidelityResolution,
                "audio.fidelity.resolution",
                { "very_low", "low", "medium", "high" }
            );

        chip.fidelityDynamics =
            nodeToEnum(
                childNode(fidelityNode, { "dynamics" }),
                chip.fidelityDynamics,
                "audio.fidelity.dynamics",
                { "fixed", "limited", "normal", "expressive" }
            );

        chip.fidelitySpace =
            nodeToEnum(
                childNode(fidelityNode, { "space" }),
                chip.fidelitySpace,
                "audio.fidelity.space",
                { "mono", "stereo" }
            );

        chip.resourcesGenerated =
            nodeToBool(
                childNode(resourcesNode, { "generated" }),
                chip.resourcesGenerated,
                "audio.resources.generated"
            );

        chip.resourcesSamples =
            nodeToBool(
                childNode(resourcesNode, { "samples" }),
                chip.resourcesSamples,
                "audio.resources.samples"
            );

        chip.resourcesStreams =
            nodeToBool(
                childNode(resourcesNode, { "streams" }),
                chip.resourcesStreams,
                "audio.resources.streams"
            );

        chip.fileAudioMode =
            nodeToEnum(
                childNode(fileAudioNode, { "mode" }),
                chip.fileAudioMode,
                "audio.fileAudio.mode",
                { "off", "sound", "music", "all" }
            );
    }

    void addMachineError(
        Diagnostics* diagnostics,
        const std::string& message,
        const std::string& file,
        const std::string& field
    )
    {
        Logger::error("machine", message);

        if (diagnostics != nullptr)
        {
            diagnostics->error(
                DiagnosticCode::MachineErrorUnclassified,
                message,
                file,
                field
            );
        }
    }

    float nodeToFloat(
        const YAML::Node& node,
        float fallback,
        const std::string& fieldName,
        float minimum,
        Diagnostics* diagnostics,
        const std::string& file
    )
    {
        if (!node || !node.IsDefined() || node.IsNull())
        {
            return fallback;
        }

        if (!node.IsScalar())
        {
            addMachineError(
                diagnostics,
                "Invalid numeric field '" + fieldName + "'",
                file,
                fieldName
            );
            return fallback;
        }

        try
        {
            const float parsedValue =
                node.as<float>();

            if (!std::isfinite(parsedValue) || parsedValue < minimum)
            {
                addMachineError(
                    diagnostics,
                    "Invalid numeric field '" + fieldName + "'",
                    file,
                    fieldName
                );
                return fallback;
            }

            return parsedValue;
        }
        catch (const YAML::Exception&)
        {
            addMachineError(
                diagnostics,
                "Invalid numeric field '" + fieldName + "'",
                file,
                fieldName
            );
            return fallback;
        }
    }

    bool unsupportedInputField(
        const YAML::Node& node,
        const std::string& field,
        Diagnostics* diagnostics,
        const std::string& file
    )
    {
        if (!hasNode(childNode(node, { field })))
        {
            return false;
        }

        addMachineError(
            diagnostics,
            "Input capability '" + field + "' is not implemented in v0.3.0",
            file,
            "input." + field
        );

        return true;
    }

    InputDirectionDefinition parseInputDirection(
        const YAML::Node& node,
        int index,
        Diagnostics* diagnostics,
        const std::string& file
    )
    {
        InputDirectionDefinition direction;

        if (!node || !node.IsDefined() || node.IsNull())
        {
            return direction;
        }

        if (!node.IsMap())
        {
            addMachineError(
                diagnostics,
                "Invalid input direction declaration",
                file,
                "input.players.controls.directions." + std::to_string(index)
            );
            return direction;
        }

        const std::string fieldPrefix =
            "input.players.controls.directions." + std::to_string(index);

        const std::string type =
            nodeToString(childNode(node, { "type" }));

        if (!type.empty())
        {
            if (type == "2way" || type == "4way")
            {
                direction.type = type;
            }
            else
            {
                direction.type = type;
                addMachineError(
                    diagnostics,
                    "Input direction type '" + type + "' is not implemented in v0.3.0",
                    file,
                    fieldPrefix + ".type"
                );
            }
        }

        const std::string simultaneous =
            nodeToString(childNode(node, { "simultaneous" }));

        if (!simultaneous.empty())
        {
            if (
                simultaneous == "first" ||
                simultaneous == "neutral" ||
                simultaneous == "last"
            )
            {
                direction.simultaneous = simultaneous;
            }
            else
            {
                addMachineError(
                    diagnostics,
                    "Invalid input direction simultaneous policy '" + simultaneous + "'",
                    file,
                    fieldPrefix + ".simultaneous"
                );
            }
        }

        direction.buffer =
            nodeToFloat(
                childNode(node, { "buffer" }),
                direction.buffer,
                fieldPrefix + ".buffer",
                0.0f,
                diagnostics,
                file
            );

        return direction;
    }

    void assignInputChip(
        InputChipDefinition& chip,
        const YAML::Node& node,
        Diagnostics* diagnostics,
        const std::string& file
    )
    {
        unsupportedInputField(node, "pointer", diagnostics, file);
        unsupportedInputField(node, "text", diagnostics, file);

        const YAML::Node systemNode =
            childNode(node, { "system" });

        const YAML::Node playersNode =
            childNode(node, { "players" });

        if (hasNode(childNode(node, { "direction" })))
        {
            addMachineError(
                diagnostics,
                "Legacy input.direction is not supported in v0.3.0",
                file,
                "input.direction"
            );
        }

        if (hasNode(childNode(node, { "buttons" })))
        {
            addMachineError(
                diagnostics,
                "Legacy input.buttons is not supported in v0.3.0",
                file,
                "input.buttons"
            );
        }

        chip.systemButtons =
            nodeToInt(
                childNode(systemNode, { "buttons" }),
                chip.systemButtons,
                "input.system.buttons",
                0
            );

        chip.players =
            nodeToInt(
                childNode(playersNode, { "count" }),
                chip.players,
                "input.players.count",
                1
            );

        const YAML::Node controlsNode =
            childNode(playersNode, { "controls" });

        chip.playerButtons =
            nodeToInt(
                childNode(controlsNode, { "buttons" }),
                chip.playerButtons,
                "input.players.controls.buttons",
                0
            );

        const YAML::Node directionsNode =
            childNode(controlsNode, { "directions" });

        if (hasNode(directionsNode))
        {
            chip.directions.clear();

            if (!directionsNode.IsSequence())
            {
                addMachineError(
                    diagnostics,
                    "input.players.controls.directions must be an array",
                    file,
                    "input.players.controls.directions"
                );
                chip.directions.push_back(InputDirectionDefinition{});
            }
            else
            {
                int index = 0;

                for (const auto& directionNode : directionsNode)
                {
                    chip.directions.push_back(
                        parseInputDirection(
                            directionNode,
                            index,
                            diagnostics,
                            file
                        )
                    );
                    ++index;
                }

                if (chip.directions.empty())
                {
                    addMachineError(
                        diagnostics,
                        "input.players.controls.directions must declare at least one direction",
                        file,
                        "input.players.controls.directions"
                    );
                    chip.directions.push_back(InputDirectionDefinition{});
                }
            }
        }
    }

    bool validMachineRoot(
        const YAML::Node& root,
        const std::string& path
    )
    {
        if (!root || !root.IsDefined())
        {
            return false;
        }

        if (!root.IsMap() || !root["machine"])
        {
            Logger::error(
                "machine",
                "Machine YAML must contain a root 'machine' block: " + path
            );

            return false;
        }

        if (!root["machine"].IsMap())
        {
            Logger::error(
                "machine",
                "Machine root must be an object: " + path
            );

            return false;
        }

        return true;
    }
}

MachineDefinition MachineLoader::defaultMachine()
{
    MachineDefinition machine;

    machine.video.screenWidth = 640;
    machine.video.screenHeight = 480;
    machine.video.outputScale = 1;
    machine.video.smoothing = false;
    machine.video.clearColor = "black";
    machine.video.colorAlpha = true;
    machine.video.planesEnabled = false;
    machine.video.objectsSprites = false;

    machine.audio.voicesMusic = 8;
    machine.audio.voicesSound = 16;
    machine.audio.voicesMode = "shared";
    machine.audio.voicesOverflow = "replace_oldest";
    machine.audio.synthesisModel = "open";
    machine.audio.synthesisTexture = "rich";
    machine.audio.synthesisMovement = "expressive";
    machine.audio.synthesisNoise = "rich";
    machine.audio.fidelityResolution = "high";
    machine.audio.fidelityDynamics = "expressive";
    machine.audio.fidelitySpace = "stereo";
    machine.audio.resourcesGenerated = true;
    machine.audio.resourcesSamples = true;
    machine.audio.resourcesStreams = true;
    machine.audio.fileAudioMode = "all";

    machine.input.systemButtons = 16;
    machine.input.players = 16;
    machine.input.directions = { InputDirectionDefinition{} };
    machine.input.playerButtons = 16;

    return machine;
}

MachineDefinition MachineLoader::load(const std::string& path)
{
    Diagnostics diagnostics;
    return load(path, diagnostics);
}

MachineDefinition MachineLoader::load(
    const std::string& path,
    Diagnostics& diagnostics
)
{
    MachineDefinition machine =
        defaultMachine();

    YAML::Node root =
        loadYamlNode(path);

    if (!validMachineRoot(root, path))
    {
        Logger::warning(
            "machine",
            "Using internal default machine"
        );

        return defaultMachine();
    }

    const YAML::Node machineNode =
        root["machine"];

    const std::string machineDirectory =
        directoryOf(path);

    const YAML::Node videoNode =
        resolveChipNode(
            machineNode["video"],
            machineDirectory,
            "video"
        );

    const YAML::Node audioNode =
        resolveChipNode(
            machineNode["audio"],
            machineDirectory,
            "audio"
        );

    const YAML::Node inputNode =
        resolveChipNode(
            machineNode["input"],
            machineDirectory,
            "input"
        );

    if (videoNode)
    {
        assignVideoChip(machine.video, videoNode);
    }

    if (audioNode)
    {
        assignAudioChip(machine.audio, audioNode);
    }

    if (inputNode)
    {
        assignInputChip(
            machine.input,
            inputNode,
            &diagnostics,
            path
        );
    }

    Logger::debug(
        "machine",
        "Loaded machine file: " + path
    );

    return machine;
}
