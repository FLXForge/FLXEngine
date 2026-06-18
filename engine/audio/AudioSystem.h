#pragma once

#include "SoundDefinition.h"

#include <raylib.h>
#include <vector>

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    void init();
    void shutdown();
    void update();

    void play(const SoundDefinition& definition);

private:
    Wave createWave(const SoundDefinition& definition) const;

    bool initialized = false;
    int sampleRate = 44100;
    std::vector<Sound> activeSounds;
};
