#pragma once

#include "SoundDefinition.h"

#include <string>
#include <vector>

struct InstrumentPlayDefinition
{
    bool legato = false;
    float glide = 0.0f;
    float vibrato = 0.0f;
};

struct InstrumentRangeDefinition
{
    std::string min = "C0";
    std::string max = "B8";
};

struct InstrumentDefinition
{
    AudioSourceDefinition source;
    AudioToneDefinition tone;
    InstrumentPlayDefinition play;
    InstrumentRangeDefinition range;
};

struct MusicChannelDefinition
{
    std::string id;
    InstrumentDefinition instrument;
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
