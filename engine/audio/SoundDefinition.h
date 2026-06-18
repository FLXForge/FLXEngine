#pragma once

#include <string>

struct SoundDefinition
{
    std::string wave = "square";
    float frequency = 440.0f;
    float duration = 0.1f;
    float volume = 1.0f;
};
