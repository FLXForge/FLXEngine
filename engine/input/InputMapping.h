#pragma once

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

struct InputMapping
{
    std::unordered_map<int, InputAction> systemButtons;
    std::unordered_map<int, PlayerMapping> players;
};
