#include "InputSystem.h"
#include "InputMappingLoader.h"
#include "../debug/Logger.h"

#include <raylib.h>

#include <algorithm>

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

bool InputSystem::loadMapping(const std::string& path)
{
    const InputMappingLoadResult result =
        InputMappingLoader::loadFile(path, chip);

    for (const Diagnostic& diagnostic : result.diagnostics.all())
    {
        Logger::warning("input", diagnostic.message);
    }

    if (!result.success)
    {
        return false;
    }

    setMapping(result.mapping);
    Logger::debug("input", "Loaded input mapping: " + result.sourceName);
    return true;
}

bool InputSystem::loadMappingContent(
    const std::string& sourceName,
    const std::string& content
)
{
    const InputMappingLoadResult result =
        InputMappingLoader::loadContent(sourceName, content, chip);

    for (const Diagnostic& diagnostic : result.diagnostics.all())
    {
        Logger::warning("input", diagnostic.message);
    }

    if (!result.success)
    {
        return false;
    }

    setMapping(result.mapping);
    Logger::debug("input", "Loaded input mapping: " + result.sourceName);
    return true;
}

void InputSystem::setMapping(const InputMapping& nextMapping)
{
    mapping = nextMapping;
    resizeRuntimeState();
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
            mapping.systemButtons.find(button);

        state.current =
            mappingIt != mapping.systemButtons.end() &&
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

        PlayerMapping& playerMapping =
            mapping.players[player];

        for (int button = 0; button < chip.playerButtons; ++button)
        {
            ButtonState& buttonState =
                state.buttons[button];

            buttonState.previous = buttonState.current;

            const auto mappingIt =
                playerMapping.buttons.find(button);

            buttonState.current =
                mappingIt != playerMapping.buttons.end() &&
                actionDown(mappingIt->second);
        }

        for (int direction = 0; direction < static_cast<int>(chip.directions.size()); ++direction)
        {
            DirectionState& directionState =
                state.directions[direction];

            directionState.previous =
                directionState.current;

            DirectionMapping directionMapping;

            if (direction < static_cast<int>(playerMapping.directions.size()))
            {
                directionMapping =
                    playerMapping.directions[direction];
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

        mapping.players[player].directions.resize(chip.directions.size());
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

int InputSystem::componentIndex(InputComponent component)
{
    return static_cast<int>(component);
}
