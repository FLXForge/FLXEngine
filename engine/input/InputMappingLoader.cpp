#include "InputMappingLoader.h"

#include <raylib.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    struct ParsedLine
    {
        std::string key;
        std::string value;
        int line = 1;
    };

    bool startsWith(const std::string& value, const std::string& prefix)
    {
        return value.rfind(prefix, 0) == 0;
    }

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

    std::vector<std::string> splitKeepingEmpty(
        const std::string& value,
        char separator
    )
    {
        std::vector<std::string> parts;
        std::stringstream stream(value);
        std::string part;

        while (std::getline(stream, part, separator))
        {
            parts.push_back(trim(part));
        }

        if (!value.empty() && value.back() == separator)
        {
            parts.push_back("");
        }

        return parts;
    }

    int parseInt(const std::string& value, int fallback = -1)
    {
        try
        {
            size_t parsed = 0;
            const int number =
                std::stoi(value, &parsed);

            return parsed == value.size()
                ? number
                : fallback;
        }
        catch (...)
        {
            return fallback;
        }
    }

    SourceRange lineRange(int line)
    {
        return SourceRange{
            SourcePosition{ line, 1 },
            SourcePosition{ line, 1 }
        };
    }

    std::unordered_map<std::string, int> buildKeyMap()
    {
        std::unordered_map<std::string, int> keys;

        for (char letter = 'A'; letter <= 'Z'; ++letter)
        {
            keys["KEY_" + std::string(1, letter)] = KEY_A + (letter - 'A');
        }

        for (char digit = '0'; digit <= '9'; ++digit)
        {
            keys["KEY_" + std::string(1, digit)] = KEY_ZERO + (digit - '0');
        }

        keys["KEY_SPACE"] = KEY_SPACE;
        keys["KEY_ENTER"] = KEY_ENTER;
        keys["KEY_ESCAPE"] = KEY_ESCAPE;
        keys["KEY_TAB"] = KEY_TAB;
        keys["KEY_BACKSPACE"] = KEY_BACKSPACE;
        keys["KEY_LEFT"] = KEY_LEFT;
        keys["KEY_RIGHT"] = KEY_RIGHT;
        keys["KEY_UP"] = KEY_UP;
        keys["KEY_DOWN"] = KEY_DOWN;
        keys["KEY_LEFT_SHIFT"] = KEY_LEFT_SHIFT;
        keys["KEY_RIGHT_SHIFT"] = KEY_RIGHT_SHIFT;
        keys["KEY_LEFT_CONTROL"] = KEY_LEFT_CONTROL;
        keys["KEY_RIGHT_CONTROL"] = KEY_RIGHT_CONTROL;
        keys["KEY_LEFT_ALT"] = KEY_LEFT_ALT;
        keys["KEY_RIGHT_ALT"] = KEY_RIGHT_ALT;
        keys["KEY_MINUS"] = KEY_MINUS;
        keys["KEY_EQUAL"] = KEY_EQUAL;
        keys["KEY_LEFT_BRACKET"] = KEY_LEFT_BRACKET;
        keys["KEY_RIGHT_BRACKET"] = KEY_RIGHT_BRACKET;
        keys["KEY_BACKSLASH"] = KEY_BACKSLASH;
        keys["KEY_SEMICOLON"] = KEY_SEMICOLON;
        keys["KEY_APOSTROPHE"] = KEY_APOSTROPHE;
        keys["KEY_GRAVE"] = KEY_GRAVE;
        keys["KEY_COMMA"] = KEY_COMMA;
        keys["KEY_PERIOD"] = KEY_PERIOD;
        keys["KEY_SLASH"] = KEY_SLASH;

        for (int index = 1; index <= 12; ++index)
        {
            keys["KEY_F" + std::to_string(index)] = KEY_F1 + index - 1;
        }

        return keys;
    }

    std::unordered_map<std::string, int> buildGamepadButtonMap()
    {
        return {
            { "A", GAMEPAD_BUTTON_RIGHT_FACE_DOWN },
            { "B", GAMEPAD_BUTTON_RIGHT_FACE_RIGHT },
            { "X", GAMEPAD_BUTTON_RIGHT_FACE_LEFT },
            { "Y", GAMEPAD_BUTTON_RIGHT_FACE_UP },
            { "START", GAMEPAD_BUTTON_MIDDLE_RIGHT },
            { "SELECT", GAMEPAD_BUTTON_MIDDLE_LEFT },
            { "UP", GAMEPAD_BUTTON_LEFT_FACE_UP },
            { "DOWN", GAMEPAD_BUTTON_LEFT_FACE_DOWN },
            { "LEFT", GAMEPAD_BUTTON_LEFT_FACE_LEFT },
            { "RIGHT", GAMEPAD_BUTTON_LEFT_FACE_RIGHT },
            { "L1", GAMEPAD_BUTTON_LEFT_TRIGGER_1 },
            { "R1", GAMEPAD_BUTTON_RIGHT_TRIGGER_1 }
        };
    }

    const std::unordered_map<std::string, int>& keyMap()
    {
        static const std::unordered_map<std::string, int> keys =
            buildKeyMap();

        return keys;
    }

    const std::unordered_map<std::string, int>& gamepadButtonMap()
    {
        static const std::unordered_map<std::string, int> buttons =
            buildGamepadButtonMap();

        return buttons;
    }

    bool validPlayer(const InputChipDefinition& chip, int player)
    {
        return player >= 1 && player <= chip.players;
    }

    bool validSystemButton(const InputChipDefinition& chip, int button)
    {
        return button >= 0 && button < chip.systemButtons;
    }

    bool validPlayerButton(const InputChipDefinition& chip, int button)
    {
        return button >= 0 && button < chip.playerButtons;
    }

    bool validDirection(const InputChipDefinition& chip, int direction)
    {
        return direction >= 0 &&
            direction < static_cast<int>(chip.directions.size());
    }

    bool parsePhysicalInput(
        const std::string& token,
        PhysicalInput& input
    )
    {
        const auto keyIt =
            keyMap().find(token);

        if (keyIt != keyMap().end())
        {
            input.type = PhysicalType::Key;
            input.code = keyIt->second;
            return true;
        }

        if (!startsWith(token, "JOY"))
        {
            return false;
        }

        const size_t separator =
            token.find('_');

        if (separator == std::string::npos || separator <= 3)
        {
            return false;
        }

        const int player =
            parseInt(token.substr(3, separator - 3));

        if (player <= 0)
        {
            return false;
        }

        const std::string buttonName =
            token.substr(separator + 1);

        const auto buttonIt =
            gamepadButtonMap().find(buttonName);

        if (buttonIt == gamepadButtonMap().end())
        {
            return false;
        }

        input.type = PhysicalType::GamepadButton;
        input.device = player - 1;
        input.code = buttonIt->second;
        return true;
    }

    InputAction parseAction(
        const ParsedLine& line,
        InputMappingLoadResult& result
    )
    {
        InputAction action;

        if (line.value.empty())
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingEmptyBinding,
                "Input mapping binding is empty",
                result.sourceName,
                line.key,
                lineRange(line.line)
            );

            return action;
        }

        for (const std::string& alternative :
            splitKeepingEmpty(line.value, ','))
        {
            if (alternative.empty())
            {
                result.diagnostics.error(
                    DiagnosticCode::InputMappingEmptyBinding,
                    "Input mapping alternative is empty",
                    result.sourceName,
                    line.key,
                    lineRange(line.line)
                );

                continue;
            }

            InputCombination combination;

            for (const std::string& token :
                splitKeepingEmpty(alternative, '+'))
            {
                if (token.empty())
                {
                    result.diagnostics.error(
                        DiagnosticCode::InputMappingEmptyBinding,
                        "Input mapping combination member is empty",
                        result.sourceName,
                        line.key,
                        lineRange(line.line)
                    );

                    combination.inputs.clear();
                    break;
                }

                PhysicalInput input;

                if (!parsePhysicalInput(token, input))
                {
                    result.diagnostics.error(
                        DiagnosticCode::InputMappingUnknownPhysicalToken,
                        "Unknown physical input token: " + token,
                        result.sourceName,
                        line.key,
                        lineRange(line.line)
                    );

                    combination.inputs.clear();
                    break;
                }

                combination.inputs.push_back(input);
            }

            if (!combination.inputs.empty())
            {
                action.alternatives.push_back(combination);
            }
        }

        return action;
    }

    InputComponent directionComponentFromString(const std::string& value)
    {
        if (value == "up")
        {
            return InputComponent::Up;
        }

        if (value == "right")
        {
            return InputComponent::Right;
        }

        if (value == "down")
        {
            return InputComponent::Down;
        }

        if (value == "left")
        {
            return InputComponent::Left;
        }

        return InputComponent::Neutral;
    }

    InputComponent normalizeComponentForDirection(
        const InputDirectionDefinition& direction,
        InputComponent component
    )
    {
        if (direction.type != "2way")
        {
            return component;
        }

        if (component == InputComponent::Up ||
            component == InputComponent::Right)
        {
            return InputComponent::Positive;
        }

        if (component == InputComponent::Down ||
            component == InputComponent::Left)
        {
            return InputComponent::Negative;
        }

        return InputComponent::Neutral;
    }

    void appendAction(InputAction& target, const InputAction& source)
    {
        target.alternatives.insert(
            target.alternatives.end(),
            source.alternatives.begin(),
            source.alternatives.end()
        );
    }

    void parseSystemButton(
        const ParsedLine& line,
        const InputChipDefinition& chip,
        InputMappingLoadResult& result
    )
    {
        const std::string indexText =
            line.key.substr(std::string("system.buttons.").size());

        const int button =
            parseInt(indexText);

        if (!validSystemButton(chip, button))
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingButtonOutOfRange,
                "System button is outside Input Chip capacity",
                result.sourceName,
                line.key,
                lineRange(line.line)
            );

            return;
        }

        result.mapping.systemButtons[button] =
            parseAction(line, result);
    }

    void parsePlayerInput(
        const ParsedLine& line,
        const InputChipDefinition& chip,
        InputMappingLoadResult& result
    )
    {
        const std::vector<std::string> parts =
            splitKeepingEmpty(line.key, '.');

        if (parts.size() < 4 || parts[0] != "players")
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingInvalidKey,
                "Invalid player input mapping key",
                result.sourceName,
                line.key,
                lineRange(line.line)
            );

            return;
        }

        const int player =
            parseInt(parts[1]);

        if (!validPlayer(chip, player))
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingPlayerOutOfRange,
                "Player is outside Input Chip capacity",
                result.sourceName,
                line.key,
                lineRange(line.line)
            );

            return;
        }

        if (parts[2] == "directions" && parts.size() == 5)
        {
            const int direction =
                parseInt(parts[3]);

            if (!validDirection(chip, direction))
            {
                result.diagnostics.error(
                    DiagnosticCode::InputMappingDirectionOutOfRange,
                    "Direction is outside Input Chip capacity",
                    result.sourceName,
                    line.key,
                    lineRange(line.line)
                );

                return;
            }

            const InputComponent sourceComponent =
                directionComponentFromString(parts[4]);

            if (sourceComponent == InputComponent::Neutral)
            {
                result.diagnostics.error(
                    DiagnosticCode::InputMappingUnknownDirectionComponent,
                    "Unknown direction component: " + parts[4],
                    result.sourceName,
                    line.key,
                    lineRange(line.line)
                );

                return;
            }

            PlayerMapping& playerMapping =
                result.mapping.players[player];

            if (playerMapping.directions.size() < chip.directions.size())
            {
                playerMapping.directions.resize(chip.directions.size());
            }

            const InputComponent targetComponent =
                normalizeComponentForDirection(
                    chip.directions[direction],
                    sourceComponent
                );

            appendAction(
                playerMapping.directions[direction].components[targetComponent],
                parseAction(line, result)
            );

            return;
        }

        if (parts[2] == "buttons" && parts.size() == 4)
        {
            const int button =
                parseInt(parts[3]);

            if (!validPlayerButton(chip, button))
            {
                result.diagnostics.error(
                    DiagnosticCode::InputMappingButtonOutOfRange,
                    "Player button is outside Input Chip capacity",
                    result.sourceName,
                    line.key,
                    lineRange(line.line)
                );

                return;
            }

            result.mapping.players[player].buttons[button] =
                parseAction(line, result);

            return;
        }

        result.diagnostics.error(
            DiagnosticCode::InputMappingInvalidKey,
            "Invalid player input mapping key",
            result.sourceName,
            line.key,
            lineRange(line.line)
        );
    }

    bool mappingHasDeclarations(const InputMapping& mapping)
    {
        if (!mapping.systemButtons.empty())
        {
            return true;
        }

        return std::any_of(
            mapping.players.begin(),
            mapping.players.end(),
            [](const auto& item)
            {
                const PlayerMapping& player =
                    item.second;

                if (!player.buttons.empty())
                {
                    return true;
                }

                return std::any_of(
                    player.directions.begin(),
                    player.directions.end(),
                    [](const DirectionMapping& direction)
                    {
                        return !direction.components.empty();
                    }
                );
            }
        );
    }
}

std::string InputMappingLoader::defaultMappingContent()
{
    return
        "players.1.directions.0.up=KEY_W,KEY_UP,JOY1_UP\n"
        "players.1.directions.0.down=KEY_S,KEY_DOWN,JOY1_DOWN\n"
        "players.1.directions.0.left=KEY_A,KEY_LEFT,JOY1_LEFT\n"
        "players.1.directions.0.right=KEY_D,KEY_RIGHT,JOY1_RIGHT\n"
        "players.1.buttons.0=KEY_SPACE,JOY1_A\n"
        "players.1.buttons.1=KEY_LEFT_CONTROL,KEY_RIGHT_CONTROL,JOY1_B\n"
        "players.1.buttons.2=KEY_LEFT_SHIFT,KEY_RIGHT_SHIFT,JOY1_X\n"
        "system.buttons.0=KEY_ENTER,JOY1_START\n"
        "system.buttons.1=KEY_ESCAPE,JOY1_SELECT\n";
}

InputMappingLoadResult InputMappingLoader::loadDefault(
    const InputChipDefinition& chip
)
{
    return loadContent(
        "<default input mapping>",
        defaultMappingContent(),
        chip
    );
}

InputMappingLoadResult InputMappingLoader::loadFile(
    const std::string& path,
    const InputChipDefinition& chip
)
{
    std::ifstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        InputMappingLoadResult result;
        result.sourceName = path;
        result.diagnostics.error(
            DiagnosticCode::InputMappingCouldNotBeOpened,
            "Input mapping could not be opened",
            path,
            "input.mapping"
        );
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return loadContent(path, buffer.str(), chip);
}

InputMappingLoadResult InputMappingLoader::loadContent(
    const std::string& sourceName,
    const std::string& content,
    const InputChipDefinition& chip
)
{
    InputMappingLoadResult result;
    result.sourceName = sourceName;
    result.content = content;

    if (content.empty())
    {
        result.diagnostics.error(
            DiagnosticCode::InputMappingEmpty,
            "Input mapping is empty",
            sourceName,
            "input.mapping"
        );
        return result;
    }

    std::unordered_set<std::string> declaredKeys;
    std::stringstream stream(content);
    std::string rawLine;
    int lineNumber = 0;

    while (std::getline(stream, rawLine))
    {
        ++lineNumber;
        const std::string line =
            trim(rawLine);

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const size_t separator =
            line.find('=');

        if (separator == std::string::npos)
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingMalformedLine,
                "Input mapping line must use key=value syntax",
                sourceName,
                "",
                lineRange(lineNumber)
            );

            continue;
        }

        ParsedLine parsed{
            trim(line.substr(0, separator)),
            trim(line.substr(separator + 1)),
            lineNumber
        };

        if (parsed.key.empty())
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingInvalidKey,
                "Input mapping key is empty",
                sourceName,
                "",
                lineRange(lineNumber)
            );

            continue;
        }

        if (!declaredKeys.insert(parsed.key).second)
        {
            result.diagnostics.error(
                DiagnosticCode::InputMappingDuplicateBinding,
                "Duplicate input mapping key",
                sourceName,
                parsed.key,
                lineRange(lineNumber)
            );

            continue;
        }

        if (startsWith(parsed.key, "system.buttons."))
        {
            parseSystemButton(parsed, chip, result);
            continue;
        }

        if (startsWith(parsed.key, "players."))
        {
            parsePlayerInput(parsed, chip, result);
            continue;
        }

        result.diagnostics.error(
            DiagnosticCode::InputMappingUnknownKey,
            "Unknown input mapping key",
            sourceName,
            parsed.key,
            lineRange(lineNumber)
        );
    }

    if (!mappingHasDeclarations(result.mapping))
    {
        result.diagnostics.error(
            DiagnosticCode::InputMappingEmpty,
            "Input mapping has no effective declarations",
            sourceName,
            "input.mapping"
        );
    }

    result.success =
        !result.diagnostics.hasErrors();

    if (!result.success)
    {
        result.mapping = InputMapping{};
    }

    return result;
}
