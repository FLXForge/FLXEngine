#include "InputSystem.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
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

    bool startsWith(const std::string& value, const std::string& prefix)
    {
        return value.rfind(prefix, 0) == 0;
    }

    int parseInt(const std::string& value, int fallback = -1)
    {
        try
        {
            return std::stoi(value);
        }
        catch (...)
        {
            return fallback;
        }
    }
}

void InputSystem::configure(const InputChipDefinition& inputChip)
{
    chip = inputChip;
}

void InputSystem::loadMapping(const std::string& path)
{
    systemButtons.clear();
    players.clear();

    if (path.empty())
    {
        Logger::warning(
            "input",
            "No input mapping defined. Normalized Input API will not receive mapped controls."
        );

        return;
    }

    std::ifstream file(path);

    if (!file.is_open())
    {
        Logger::warning(
            "input",
            "Input mapping could not be opened: " + path
        );

        return;
    }

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;
        line = trim(line);

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const size_t separator =
            line.find('=');

        if (separator == std::string::npos)
        {
            Logger::warning(
                "input",
                "Ignoring invalid mapping line " + std::to_string(lineNumber)
            );

            continue;
        }

        parseLine(
            trim(line.substr(0, separator)),
            trim(line.substr(separator + 1)),
            lineNumber
        );
    }

    Logger::debug(
        "input",
        "Loaded input mapping: " + path
    );
}

void InputSystem::update(
    float delta,
    int screenWidth,
    int screenHeight,
    Rectangle screenArea
)
{
    if (chip.pointer)
    {
        const float scaleX =
            screenArea.width <= 0.0f
            ? 1.0f
            : static_cast<float>(screenWidth) / screenArea.width;

        const float scaleY =
            screenArea.height <= 0.0f
            ? 1.0f
            : static_cast<float>(screenHeight) / screenArea.height;

        mousePointer.x =
            (static_cast<float>(GetMouseX()) - screenArea.x) * scaleX;

        mousePointer.y =
            (static_cast<float>(GetMouseY()) - screenArea.y) * scaleY;

        mousePointer.x =
            std::clamp(mousePointer.x, 0.0f, static_cast<float>(screenWidth));

        mousePointer.y =
            std::clamp(mousePointer.y, 0.0f, static_cast<float>(screenHeight));

        return;
    }

    if (!virtualPointerInitialized)
    {
        virtualPointer = Vector2{
            static_cast<float>(screenWidth) * 0.5f,
            static_cast<float>(screenHeight) * 0.5f
        };
        virtualPointerInitialized = true;
    }

    constexpr float speed = 160.0f;

    if (playerLeft(1))
    {
        virtualPointer.x -= speed * delta;
    }

    if (playerRight(1))
    {
        virtualPointer.x += speed * delta;
    }

    if (playerUp(1))
    {
        virtualPointer.y -= speed * delta;
    }

    if (playerDown(1))
    {
        virtualPointer.y += speed * delta;
    }

    virtualPointer.x =
        std::clamp(virtualPointer.x, 0.0f, static_cast<float>(screenWidth));

    virtualPointer.y =
        std::clamp(virtualPointer.y, 0.0f, static_cast<float>(screenHeight));
}

bool InputSystem::systemDown(int button) const
{
    const auto it =
        systemButtons.find(button);

    return it != systemButtons.end() && actionDown(it->second);
}

bool InputSystem::systemPressed(int button) const
{
    const auto it =
        systemButtons.find(button);

    return it != systemButtons.end() && actionPressed(it->second);
}

bool InputSystem::playerUp(int player) const
{
    const auto it = players.find(player);
    return it != players.end() && actionDown(it->second.up);
}

bool InputSystem::playerDown(int player) const
{
    const auto it = players.find(player);
    return it != players.end() && actionDown(it->second.down);
}

bool InputSystem::playerLeft(int player) const
{
    const auto it = players.find(player);
    return it != players.end() && actionDown(it->second.left);
}

bool InputSystem::playerRight(int player) const
{
    const auto it = players.find(player);
    return it != players.end() && actionDown(it->second.right);
}

bool InputSystem::playerButtonDown(int player, int button) const
{
    const auto playerIt = players.find(player);

    if (playerIt == players.end())
    {
        return false;
    }

    const auto buttonIt =
        playerIt->second.buttons.find(button);

    return buttonIt != playerIt->second.buttons.end() &&
        actionDown(buttonIt->second);
}

bool InputSystem::playerButtonPressed(int player, int button) const
{
    const auto playerIt = players.find(player);

    if (playerIt == players.end())
    {
        return false;
    }

    const auto buttonIt =
        playerIt->second.buttons.find(button);

    return buttonIt != playerIt->second.buttons.end() &&
        actionPressed(buttonIt->second);
}

float InputSystem::pointerX() const
{
    return chip.pointer ? mousePointer.x : virtualPointer.x;
}

float InputSystem::pointerY() const
{
    return chip.pointer ? mousePointer.y : virtualPointer.y;
}

bool InputSystem::pointerDown(int button) const
{
    if (chip.pointer)
    {
        return IsMouseButtonDown(button);
    }

    return button == 0 && playerButtonDown(1, 0);
}

bool InputSystem::pointerPressed(int button) const
{
    if (chip.pointer)
    {
        return IsMouseButtonPressed(button);
    }

    return button == 0 && playerButtonPressed(1, 0);
}

bool InputSystem::actionDown(const InputAction& action) const
{
    return std::any_of(
        action.alternatives.begin(),
        action.alternatives.end(),
        [this](const InputCombination& combination)
        {
            return combinationDown(combination);
        }
    );
}

bool InputSystem::actionPressed(const InputAction& action) const
{
    return std::any_of(
        action.alternatives.begin(),
        action.alternatives.end(),
        [this](const InputCombination& combination)
        {
            return combinationPressed(combination);
        }
    );
}

bool InputSystem::combinationDown(const InputCombination& combination) const
{
    return !combination.inputs.empty() &&
        std::all_of(
            combination.inputs.begin(),
            combination.inputs.end(),
            [this](const PhysicalInput& input)
            {
                return physicalDown(input);
            }
        );
}

bool InputSystem::combinationPressed(
    const InputCombination& combination
) const
{
    if (!combinationDown(combination))
    {
        return false;
    }

    return std::any_of(
        combination.inputs.begin(),
        combination.inputs.end(),
        [this](const PhysicalInput& input)
        {
            return physicalPressed(input);
        }
    );
}

bool InputSystem::physicalDown(const PhysicalInput& input) const
{
    if (input.type == PhysicalType::Key)
    {
        return IsKeyDown(input.code);
    }

    return IsGamepadAvailable(input.device) &&
        IsGamepadButtonDown(input.device, input.code);
}

bool InputSystem::physicalPressed(const PhysicalInput& input) const
{
    if (input.type == PhysicalType::Key)
    {
        return IsKeyPressed(input.code);
    }

    return IsGamepadAvailable(input.device) &&
        IsGamepadButtonPressed(input.device, input.code);
}

void InputSystem::parseLine(
    const std::string& key,
    const std::string& value,
    int lineNumber
)
{
    if (startsWith(key, "system.buttons."))
    {
        parseSystemButton(key, value, lineNumber);
        return;
    }

    if (startsWith(key, "players."))
    {
        parsePlayerInput(key, value, lineNumber);
        return;
    }

    Logger::warning(
        "input",
        "Ignoring unknown mapping key '" + key +
        "' at line " + std::to_string(lineNumber)
    );
}

void InputSystem::parseSystemButton(
    const std::string& key,
    const std::string& value,
    int lineNumber
)
{
    const std::string indexText =
        key.substr(std::string("system.buttons.").size());

    const int button =
        parseInt(indexText);

    if (!validSystemButton(button, lineNumber))
    {
        return;
    }

    systemButtons[button] =
        parseAction(value, lineNumber);
}

void InputSystem::parsePlayerInput(
    const std::string& key,
    const std::string& value,
    int lineNumber
)
{
    const std::vector<std::string> parts =
        split(key, '.');

    if (parts.size() < 4 || parts[0] != "players")
    {
        Logger::warning(
            "input",
            "Ignoring invalid player mapping at line " +
            std::to_string(lineNumber)
        );

        return;
    }

    const int player =
        parseInt(parts[1]);

    if (!validPlayer(player, lineNumber))
    {
        return;
    }

    if (parts[2] == "direction" && parts.size() == 4)
    {
        if (!directionMappingAllowed(lineNumber))
        {
            return;
        }

        InputAction action =
            parseAction(value, lineNumber);

        if (parts[3] == "up")
        {
            players[player].up = action;
        }
        else if (parts[3] == "down")
        {
            players[player].down = action;
        }
        else if (parts[3] == "left")
        {
            players[player].left = action;
        }
        else if (parts[3] == "right")
        {
            players[player].right = action;
        }
        else
        {
            Logger::warning(
                "input",
                "Ignoring unknown direction '" + parts[3] +
                "' at line " + std::to_string(lineNumber)
            );
        }

        return;
    }

    if (parts[2] == "buttons" && parts.size() == 4)
    {
        const int button =
            parseInt(parts[3]);

        if (!validPlayerButton(button, lineNumber))
        {
            return;
        }

        players[player].buttons[button] =
            parseAction(value, lineNumber);

        return;
    }

    Logger::warning(
        "input",
        "Ignoring invalid player mapping at line " +
        std::to_string(lineNumber)
    );
}

InputSystem::InputAction InputSystem::parseAction(
    const std::string& value,
    int lineNumber
) const
{
    InputAction action;

    for (const std::string& alternative : split(value, ','))
    {
        InputCombination combination;

        for (const std::string& token : split(alternative, '+'))
        {
            PhysicalInput input;

            if (!parsePhysicalInput(token, input))
            {
                Logger::warning(
                    "input",
                    "Ignoring unknown input token '" + token +
                    "' at line " + std::to_string(lineNumber)
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

bool InputSystem::parsePhysicalInput(
    const std::string& token,
    PhysicalInput& input
) const
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

bool InputSystem::validPlayer(int player, int lineNumber) const
{
    if (player <= 0 || player > chip.players)
    {
        Logger::warning(
            "input",
            "Ignoring player mapping outside Input Chip players at line " +
            std::to_string(lineNumber)
        );

        return false;
    }

    return true;
}

bool InputSystem::validPlayerButton(int button, int lineNumber) const
{
    if (button < 0 || button >= chip.playerButtons)
    {
        Logger::warning(
            "input",
            "Ignoring player button outside Input Chip limit at line " +
            std::to_string(lineNumber)
        );

        return false;
    }

    return true;
}

bool InputSystem::validSystemButton(int button, int lineNumber) const
{
    if (button < 0 || button >= chip.systemButtons)
    {
        Logger::warning(
            "input",
            "Ignoring system button outside Input Chip limit at line " +
            std::to_string(lineNumber)
        );

        return false;
    }

    return true;
}

bool InputSystem::directionMappingAllowed(int lineNumber) const
{
    if (chip.direction == "none")
    {
        Logger::warning(
            "input",
            "Ignoring direction mapping because Input Chip direction is none at line " +
            std::to_string(lineNumber)
        );

        return false;
    }

    return true;
}

std::string InputSystem::trim(const std::string& value)
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

std::vector<std::string> InputSystem::split(
    const std::string& value,
    char separator
)
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;

    while (std::getline(stream, part, separator))
    {
        part = trim(part);

        if (!part.empty())
        {
            parts.push_back(part);
        }
    }

    return parts;
}
