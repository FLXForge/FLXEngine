#pragma once

#include "../machine/MachineDefinition.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

enum class InputComponent
{
    Neutral = 0,
    Negative,
    Positive,
    Up,
    Right,
    Down,
    Left
};

enum class InputSubjectKind
{
    Player,
    System
};

struct InputSubject
{
    InputSubjectKind kind = InputSubjectKind::Player;
    int index = 1;
};

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
    void loadMapping(const std::string& path);
    void loadMappingContent(
        const std::string& sourceName,
        const std::string& content
    );
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

    struct DirectionMapping
    {
        std::unordered_map<InputComponent, InputAction> components;
    };

    struct PlayerMapping
    {
        std::vector<DirectionMapping> directions;
        std::unordered_map<int, InputAction> buttons;
    };

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

    bool mappingPlayerValid(int player, int lineNumber) const;
    bool mappingPlayerButtonValid(int button, int lineNumber) const;
    bool mappingSystemButtonValid(int button, int lineNumber) const;
    bool mappingDirectionValid(int direction, int lineNumber) const;
    bool mappingComponentValid(
        int direction,
        InputComponent component,
        int lineNumber
    ) const;

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

    static std::string trim(const std::string& value);
    static std::vector<std::string> split(
        const std::string& value,
        char separator
    );
    static InputComponent componentFromString(const std::string& value);
    static int componentIndex(InputComponent component);

    InputChipDefinition chip;
    std::unordered_map<int, InputAction> systemButtons;
    std::unordered_map<int, PlayerMapping> players;
    std::unordered_map<int, ButtonState> systemButtonStates;
    std::unordered_map<int, PlayerState> playerStates;
    PhysicalInputProvider* provider = nullptr;
};
