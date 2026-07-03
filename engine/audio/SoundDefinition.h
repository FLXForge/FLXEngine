#pragma once

#include <string>

struct SoundDefinition
{
    std::string wave = "square";
    std::string note;
    float frequency = 440.0f;
    bool hasFrequency = false;
    float duration = 0.1f;
    float volume = 1.0f;
};
