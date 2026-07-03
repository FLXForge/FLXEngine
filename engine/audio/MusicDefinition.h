#pragma once

#include <string>
#include <vector>

struct MusicChannelDefinition
{
    std::string id;
    std::string wave = "square";
    float volume = 1.0f;
    std::string length = "1/4";
    std::vector<std::string> notes;
};

struct MusicDefinition
{
    float tempo = 120.0f;
    bool loop = false;
    std::vector<MusicChannelDefinition> channels;
};
