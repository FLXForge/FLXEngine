#pragma once

#include "MusicDefinition.h"
#include "../machine/MachineDefinition.h"
#include <cstddef>
#include <cstdint>
#include <vector>

// Internal, device-independent synthesizer. Owns the prepared score, never full PCM.
class MusicSequencer
{
public:
    MusicSequencer(const MusicDefinition& definition, int channelCount,
        const AudioChipDefinition& chip, int sampleRate = 44100);
    size_t generate(short* samples, size_t capacity, bool loop);
    bool finished() const { return cursor == sampleCount; }
    uint64_t frameCount() const { return sampleCount; }
private:
    struct PreparedNote
    {
        bool rest = true;
        float frequency = 440.0f;
    };
    struct PreparedChannel
    {
        std::string id;
        AudioSourceDefinition source;
        AudioToneDefinition tone;
        InstrumentPlayDefinition play;
        float volume = 1.0f;
        float stepDuration = 0.5f;
        std::vector<PreparedNote> notes;
    };
    AudioChipDefinition chip;
    int sampleRate;
    std::vector<PreparedChannel> preparedChannels;
    std::vector<float> history;
    uint64_t cursor = 0;
    uint64_t sampleCount = 0;
};
