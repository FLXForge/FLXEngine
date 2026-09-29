#pragma once

#include "InputMapping.h"
#include "../machine/MachineDefinition.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

class PhysicalInputProvider
{
public:
    virtual ~PhysicalInputProvider() = default;

    virtual bool keyDown(int key) const = 0;
    virtual bool keyPressed(int key) const = 0;
    virtual bool gamepadButtonDown(int device, int button) const = 0;
    virtual bool gamepadButtonPressed(int device, int button) const = 0;
};

class InputSystem
{
public:
    void configure(const InputChipDefinition& inputChip);
    void setPhysicalInputProvider(PhysicalInputProvider* nextProvider);
    void setMapping(const InputMapping& nextMapping);
    void update(float delta);

    bool systemButtonPressed(int button) const;
    bool systemButtonDown(int button) const;
    bool systemButtonReleased(int button) const;

    bool playerButtonPressed(int player, int button) const;
    bool playerButtonDown(int player, int button) const;
    bool playerButtonReleased(int player, int button) const;

    bool playerDirectionPressed(
        int player,
        int direction,
        InputComponent component
    ) const;
    bool playerDirectionDown(
        int player,
        int direction,
        InputComponent component
    ) const;
    bool playerDirectionReleased(
        int player,
        int direction,
        InputComponent component
    ) const;

    bool validPlayer(int player) const;
    bool validSystemButton(int button) const;
    bool validPlayerButton(int button) const;
    bool validDirection(int direction) const;
    bool componentAllowed(int direction, InputComponent component) const;

private:
    struct ButtonState
    {
        bool previous = false;
        bool current = false;
    };

    struct DirectionState
    {
        InputComponent previous = InputComponent::Neutral;
        InputComponent current = InputComponent::Neutral;
        InputComponent pending = InputComponent::Neutral;
        float pendingLeft = 0.0f;
        int nextOrder = 1;
        std::array<bool, 7> physical = {};
        std::array<int, 7> order = {};
    };

    struct PlayerState
    {
        std::vector<DirectionState> directions;
        std::unordered_map<int, ButtonState> buttons;
    };

    bool actionDown(const InputAction& action) const;
    bool actionPressed(const InputAction& action) const;
    bool combinationDown(const InputCombination& combination) const;
    bool combinationPressed(const InputCombination& combination) const;
    bool physicalDown(const PhysicalInput& input) const;
    bool physicalPressed(const PhysicalInput& input) const;

    void resizeRuntimeState();
    ButtonState buttonState(
        const std::unordered_map<int, ButtonState>& buttons,
        int button
    ) const;

    DirectionState directionState(
        int player,
        int direction
    ) const;

    InputComponent resolveDirection(
        const InputDirectionDefinition& definition,
        const DirectionMapping& mapping,
        DirectionState& state,
        float delta
    ) const;

    InputComponent resolveLast(
        const std::vector<InputComponent>& active,
        const DirectionState& state
    ) const;

    InputComponent resolveNeutral(
        const std::vector<InputComponent>& active
    ) const;

    InputComponent resolveFirst(
        const std::vector<InputComponent>& active,
        const InputDirectionDefinition& definition,
        DirectionState& state,
        float delta
    ) const;

    bool componentMatches(
        const InputDirectionDefinition& definition,
        InputComponent value,
        InputComponent query
    ) const;

    static int componentIndex(InputComponent component);

    InputChipDefinition chip;
    InputMapping mapping;
    std::unordered_map<int, ButtonState> systemButtonStates;
    std::unordered_map<int, PlayerState> playerStates;
    PhysicalInputProvider* provider = nullptr;
};
