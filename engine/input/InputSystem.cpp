#include "InputSystem.h"
#include "../debug/Logger.h"

#include <raylib.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace
{
    class RaylibPhysicalInputProvider : public PhysicalInputProvider
    {
    public:
        bool keyDown(int key) const override
        {
            return IsKeyDown(key);
        }

        bool keyPressed(int key) const override
        {
            return IsKeyPressed(key);
        }

        bool gamepadButtonDown(int device, int button) const override
        {
            return IsGamepadAvailable(device) &&
                IsGamepadButtonDown(device, button);
        }

        bool gamepadButtonPressed(int device, int button) const override
        {
            return IsGamepadAvailable(device) &&
                IsGamepadButtonPressed(device, button);
        }
    };

    RaylibPhysicalInputProvider defaultProvider;

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
    resizeRuntimeState();
}

void InputSystem::setPhysicalInputProvider(
    PhysicalInputProvider* nextProvider
)
{
    provider = nextProvider;
}

void InputSystem::loadMapping(const std::string& path)
{
    systemButtons.clear();
    players.clear();

    if (path.empty())
    {
        Logger::warning(
            "input",
            "No input mapping defined. Functional input API will not receive mapped controls."
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

    std::stringstream buffer;
    buffer << file.rdbuf();
    loadMappingContent(path, buffer.str());
}

void InputSystem::loadMappingContent(
    const std::string& sourceName,
    const std::string& content
)
{
    systemButtons.clear();
    players.clear();

    if (content.empty())
    {
        Logger::warning(
            "input",
            "No input mapping defined. Functional input API will not receive mapped controls."
        );

        return;
    }

    std::stringstream stream(content);
    std::string line;
    int lineNumber = 0;

    while (std::getline(stream, line))
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

    resizeRuntimeState();

    Logger::debug(
        "input",
        "Loaded input mapping: " + sourceName
    );
}

void InputSystem::update(float delta)
{
    const PhysicalInputProvider* activeProvider =
        provider == nullptr
        ? &defaultProvider
        : provider;

    for (int button = 0; button < chip.systemButtons; ++button)
    {
        ButtonState& state =
            systemButtonStates[button];

        state.previous = state.current;

        const auto mappingIt =
            systemButtons.find(button);

        state.current =
            mappingIt != systemButtons.end() &&
            std::any_of(
                mappingIt->second.alternatives.begin(),
                mappingIt->second.alternatives.end(),
                [this, activeProvider](const InputCombination& combination)
                {
                    return !combination.inputs.empty() &&
                        std::all_of(
                            combination.inputs.begin(),
                            combination.inputs.end(),
                            [this, activeProvider](const PhysicalInput& input)
                            {
                                if (input.type == PhysicalType::Key)
                                {
                                    return activeProvider->keyDown(input.code);
                                }

                                return activeProvider->gamepadButtonDown(
                                    input.device,
                                    input.code
                                );
                            }
                        );
                }
            );
    }

    for (int player = 1; player <= chip.players; ++player)
    {
        PlayerState& state =
            playerStates[player];

        PlayerMapping& mapping =
            players[player];

        for (int button = 0; button < chip.playerButtons; ++button)
        {
            ButtonState& buttonState =
                state.buttons[button];

            buttonState.previous = buttonState.current;

            const auto mappingIt =
                mapping.buttons.find(button);

            buttonState.current =
                mappingIt != mapping.buttons.end() &&
                actionDown(mappingIt->second);
        }

        for (int direction = 0; direction < static_cast<int>(chip.directions.size()); ++direction)
        {
            DirectionState& directionState =
                state.directions[direction];

            directionState.previous =
                directionState.current;

            DirectionMapping directionMapping;

            if (direction < static_cast<int>(mapping.directions.size()))
            {
                directionMapping =
                    mapping.directions[direction];
            }

            directionState.current =
                resolveDirection(
                    chip.directions[direction],
                    directionMapping,
                    directionState,
                    delta
                );
        }
    }
}

bool InputSystem::systemButtonPressed(int button) const
{
    const ButtonState state =
        buttonState(systemButtonStates, button);

    return !state.previous && state.current;
}

bool InputSystem::systemButtonDown(int button) const
{
    return buttonState(systemButtonStates, button).current;
}

bool InputSystem::systemButtonReleased(int button) const
{
    const ButtonState state =
        buttonState(systemButtonStates, button);

    return state.previous && !state.current;
}

bool InputSystem::playerButtonPressed(int player, int button) const
{
    const auto playerIt =
        playerStates.find(player);

    if (playerIt == playerStates.end())
    {
        return false;
    }

    const ButtonState state =
        buttonState(playerIt->second.buttons, button);

    return !state.previous && state.current;
}

bool InputSystem::playerButtonDown(int player, int button) const
{
    const auto playerIt =
        playerStates.find(player);

    return playerIt != playerStates.end() &&
        buttonState(playerIt->second.buttons, button).current;
}

bool InputSystem::playerButtonReleased(int player, int button) const
{
    const auto playerIt =
        playerStates.find(player);

    if (playerIt == playerStates.end())
    {
        return false;
    }

    const ButtonState state =
        buttonState(playerIt->second.buttons, button);

    return state.previous && !state.current;
}

bool InputSystem::playerDirectionPressed(
    int player,
    int direction,
    InputComponent component
) const
{
    if (!validDirection(direction))
    {
        return false;
    }

    const DirectionState state =
        directionState(player, direction);

    const InputDirectionDefinition& definition =
        chip.directions[direction];

    return !componentMatches(definition, state.previous, component) &&
        componentMatches(definition, state.current, component);
}

bool InputSystem::playerDirectionDown(
    int player,
    int direction,
    InputComponent component
) const
{
    if (!validDirection(direction))
    {
        return false;
    }

    const DirectionState state =
        directionState(player, direction);

    return componentMatches(
        chip.directions[direction],
        state.current,
        component
    );
}

bool InputSystem::playerDirectionReleased(
    int player,
    int direction,
    InputComponent component
) const
{
    if (!validDirection(direction))
    {
        return false;
    }

    const DirectionState state =
        directionState(player, direction);

    const InputDirectionDefinition& definition =
        chip.directions[direction];

    return componentMatches(definition, state.previous, component) &&
        !componentMatches(definition, state.current, component);
}

bool InputSystem::validPlayer(int player) const
{
    return player >= 1 && player <= chip.players;
}

bool InputSystem::validSystemButton(int button) const
{
    return button >= 0 && button < chip.systemButtons;
}

bool InputSystem::validPlayerButton(int button) const
{
    return button >= 0 && button < chip.playerButtons;
}

bool InputSystem::validDirection(int direction) const
{
    return direction >= 0 &&
        direction < static_cast<int>(chip.directions.size());
}

bool InputSystem::componentAllowed(
    int direction,
    InputComponent component
) const
{
    if (!validDirection(direction))
    {
        return false;
    }

    const std::string& type =
        chip.directions[direction].type;

    if (component == InputComponent::Negative || component == InputComponent::Positive)
    {
        return type == "2way" || type == "4way";
    }

    if (
        component == InputComponent::Up ||
        component == InputComponent::Right ||
        component == InputComponent::Down ||
        component == InputComponent::Left
    )
    {
        return type == "4way";
    }

    return false;
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
    const PhysicalInputProvider* activeProvider =
        provider == nullptr
        ? &defaultProvider
        : provider;

    if (input.type == PhysicalType::Key)
    {
        return activeProvider->keyDown(input.code);
    }

    return activeProvider->gamepadButtonDown(input.device, input.code);
}

bool InputSystem::physicalPressed(const PhysicalInput& input) const
{
    const PhysicalInputProvider* activeProvider =
        provider == nullptr
        ? &defaultProvider
        : provider;

    if (input.type == PhysicalType::Key)
    {
        return activeProvider->keyPressed(input.code);
    }

    return activeProvider->gamepadButtonPressed(input.device, input.code);
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

    if (!mappingSystemButtonValid(button, lineNumber))
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

    if (!mappingPlayerValid(player, lineNumber))
    {
        return;
    }

    if (parts[2] == "directions" && parts.size() == 5)
    {
        const int direction =
            parseInt(parts[3]);

        const InputComponent component =
            componentFromString(parts[4]);

        if (
            !mappingDirectionValid(direction, lineNumber) ||
            !mappingComponentValid(direction, component, lineNumber)
        )
        {
            return;
        }

        PlayerMapping& playerMapping =
            players[player];

        if (playerMapping.directions.size() < chip.directions.size())
        {
            playerMapping.directions.resize(chip.directions.size());
        }

        playerMapping.directions[direction].components[component] =
            parseAction(value, lineNumber);

        return;
    }

    if (parts[2] == "buttons" && parts.size() == 4)
    {
        const int button =
            parseInt(parts[3]);

        if (!mappingPlayerButtonValid(button, lineNumber))
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

bool InputSystem::mappingPlayerValid(int player, int lineNumber) const
{
    if (!validPlayer(player))
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

bool InputSystem::mappingPlayerButtonValid(int button, int lineNumber) const
{
    if (!validPlayerButton(button))
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

bool InputSystem::mappingSystemButtonValid(int button, int lineNumber) const
{
    if (!validSystemButton(button))
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

bool InputSystem::mappingDirectionValid(int direction, int lineNumber) const
{
    if (!validDirection(direction))
    {
        Logger::warning(
            "input",
            "Ignoring direction outside Input Chip limit at line " +
            std::to_string(lineNumber)
        );

        return false;
    }

    return true;
}

bool InputSystem::mappingComponentValid(
    int direction,
    InputComponent component,
    int lineNumber
) const
{
    if (!componentAllowed(direction, component))
    {
        Logger::warning(
            "input",
            "Ignoring incompatible direction component at line " +
            std::to_string(lineNumber)
        );

        return false;
    }

    return true;
}

void InputSystem::resizeRuntimeState()
{
    for (int button = 0; button < chip.systemButtons; ++button)
    {
        systemButtonStates.try_emplace(button);
    }

    for (int player = 1; player <= chip.players; ++player)
    {
        PlayerState& state =
            playerStates[player];

        state.directions.resize(chip.directions.size());

        for (int button = 0; button < chip.playerButtons; ++button)
        {
            state.buttons.try_emplace(button);
        }

        players[player].directions.resize(chip.directions.size());
    }
}

InputSystem::ButtonState InputSystem::buttonState(
    const std::unordered_map<int, ButtonState>& buttons,
    int button
) const
{
    const auto it =
        buttons.find(button);

    return it == buttons.end()
        ? ButtonState{}
        : it->second;
}

InputSystem::DirectionState InputSystem::directionState(
    int player,
    int direction
) const
{
    const auto playerIt =
        playerStates.find(player);

    if (
        playerIt == playerStates.end() ||
        direction < 0 ||
        direction >= static_cast<int>(playerIt->second.directions.size())
    )
    {
        return DirectionState{};
    }

    return playerIt->second.directions[direction];
}

InputComponent InputSystem::resolveDirection(
    const InputDirectionDefinition& definition,
    const DirectionMapping& mapping,
    DirectionState& state,
    float delta
) const
{
    std::vector<InputComponent> active;

    const std::vector<InputComponent> components =
        definition.type == "2way"
        ? std::vector<InputComponent>{ InputComponent::Negative, InputComponent::Positive }
        : std::vector<InputComponent>{
            InputComponent::Up,
            InputComponent::Right,
            InputComponent::Down,
            InputComponent::Left
        };

    for (InputComponent component : components)
    {
        const int index =
            componentIndex(component);

        const auto mappingIt =
            mapping.components.find(component);

        const bool isDown =
            mappingIt != mapping.components.end() &&
            actionDown(mappingIt->second);

        if (isDown && !state.physical[index])
        {
            state.order[index] =
                state.nextOrder++;
        }

        state.physical[index] =
            isDown;

        if (isDown)
        {
            active.push_back(component);
        }
    }

    if (definition.simultaneous == "first")
    {
        return resolveFirst(active, definition, state, delta);
    }

    if (definition.simultaneous == "neutral")
    {
        state.pending = InputComponent::Neutral;
        state.pendingLeft = 0.0f;
        return resolveNeutral(active);
    }

    state.pending = InputComponent::Neutral;
    state.pendingLeft = 0.0f;
    return resolveLast(active, state);
}

InputComponent InputSystem::resolveLast(
    const std::vector<InputComponent>& active,
    const DirectionState& state
) const
{
    InputComponent result =
        InputComponent::Neutral;

    int bestOrder = -1;

    for (InputComponent component : active)
    {
        const int order =
            state.order[componentIndex(component)];

        if (order > bestOrder)
        {
            bestOrder = order;
            result = component;
        }
    }

    return result;
}

InputComponent InputSystem::resolveNeutral(
    const std::vector<InputComponent>& active
) const
{
    return active.size() == 1
        ? active.front()
        : InputComponent::Neutral;
}

InputComponent InputSystem::resolveFirst(
    const std::vector<InputComponent>& active,
    const InputDirectionDefinition& definition,
    DirectionState& state,
    float delta
) const
{
    const auto isActive =
        [&active](InputComponent component)
        {
            return std::find(active.begin(), active.end(), component) != active.end();
        };

    if (state.pending != InputComponent::Neutral)
    {
        state.pendingLeft =
            std::max(0.0f, state.pendingLeft - delta);

        if (state.pendingLeft <= 0.0f || !isActive(state.pending))
        {
            state.pending = InputComponent::Neutral;
            state.pendingLeft = 0.0f;
        }
    }

    for (InputComponent component : active)
    {
        if (
            state.current != InputComponent::Neutral &&
            component != state.current &&
            state.physical[componentIndex(component)] &&
            state.order[componentIndex(component)] == state.nextOrder - 1 &&
            definition.buffer > 0.0f
        )
        {
            state.pending = component;
            state.pendingLeft = definition.buffer;
        }
    }

    if (
        state.current != InputComponent::Neutral &&
        isActive(state.current)
    )
    {
        return state.current;
    }

    if (
        state.pending != InputComponent::Neutral &&
        isActive(state.pending)
    )
    {
        const InputComponent pending =
            state.pending;

        state.pending = InputComponent::Neutral;
        state.pendingLeft = 0.0f;
        return pending;
    }

    InputComponent result =
        InputComponent::Neutral;

    int bestOrder = 0;

    for (InputComponent component : active)
    {
        const int order =
            state.order[componentIndex(component)];

        if (bestOrder == 0 || order < bestOrder)
        {
            bestOrder = order;
            result = component;
        }
    }

    return result;
}

bool InputSystem::componentMatches(
    const InputDirectionDefinition& definition,
    InputComponent value,
    InputComponent query
) const
{
    if (query == InputComponent::Negative)
    {
        if (definition.type == "2way")
        {
            return value == InputComponent::Negative;
        }

        if (definition.type == "4way")
        {
            return value == InputComponent::Down ||
                value == InputComponent::Left;
        }
    }

    if (query == InputComponent::Positive)
    {
        if (definition.type == "2way")
        {
            return value == InputComponent::Positive;
        }

        if (definition.type == "4way")
        {
            return value == InputComponent::Up ||
                value == InputComponent::Right;
        }
    }

    return value == query;
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

InputComponent InputSystem::componentFromString(const std::string& value)
{
    if (value == "negative")
    {
        return InputComponent::Negative;
    }

    if (value == "positive")
    {
        return InputComponent::Positive;
    }

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

int InputSystem::componentIndex(InputComponent component)
{
    return static_cast<int>(component);
}
