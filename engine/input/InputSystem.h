#pragma once

#include "../machine/MachineDefinition.h"

#include <raylib.h>

#include <string>
#include <unordered_map>
#include <vector>

class InputSystem
{
public:
    void configure(const InputChipDefinition& inputChip);
    void loadMapping(const std::string& path);
    void update(
        float delta,
        int screenWidth,
        int screenHeight,
        Rectangle screenArea
    );

    bool systemDown(int button) const;
    bool systemPressed(int button) const;

    bool playerUp(int player) const;
    bool playerDown(int player) const;
    bool playerLeft(int player) const;
    bool playerRight(int player) const;
    bool playerButtonDown(int player, int button) const;
    bool playerButtonPressed(int player, int button) const;

    float pointerX() const;
    float pointerY() const;
    bool pointerDown(int button) const;
    bool pointerPressed(int button) const;

private:
    enum class PhysicalType
    {
        Key,
        GamepadButton
    };

    struct PhysicalInput
    {
        PhysicalType type = PhysicalType::Key;
        int device = 0;
        int code = 0;
    };

    struct InputCombination
    {
        std::vector<PhysicalInput> inputs;
    };

    struct InputAction
    {
        std::vector<InputCombination> alternatives;
    };

    struct PlayerMapping
    {
        InputAction up;
        InputAction down;
        InputAction left;
        InputAction right;
        std::unordered_map<int, InputAction> buttons;
    };

    bool actionDown(const InputAction& action) const;
    bool actionPressed(const InputAction& action) const;
    bool combinationDown(const InputCombination& combination) const;
    bool combinationPressed(const InputCombination& combination) const;
    bool physicalDown(const PhysicalInput& input) const;
    bool physicalPressed(const PhysicalInput& input) const;

    void parseLine(
        const std::string& key,
        const std::string& value,
        int lineNumber
    );

    void parseSystemButton(
        const std::string& key,
        const std::string& value,
        int lineNumber
    );

    void parsePlayerInput(
        const std::string& key,
        const std::string& value,
        int lineNumber
    );

    InputAction parseAction(
        const std::string& value,
        int lineNumber
    ) const;

    bool parsePhysicalInput(
        const std::string& token,
        PhysicalInput& input
    ) const;

    bool validPlayer(int player, int lineNumber) const;
    bool validPlayerButton(int button, int lineNumber) const;
    bool validSystemButton(int button, int lineNumber) const;
    bool directionMappingAllowed(int lineNumber) const;

    static std::string trim(const std::string& value);
    static std::vector<std::string> split(
        const std::string& value,
        char separator
    );

    InputChipDefinition chip;
    std::unordered_map<int, InputAction> systemButtons;
    std::unordered_map<int, PlayerMapping> players;
    Vector2 mousePointer = Vector2{ 0.0f, 0.0f };
    Vector2 virtualPointer = Vector2{ 0.0f, 0.0f };
    bool virtualPointerInitialized = false;
};
