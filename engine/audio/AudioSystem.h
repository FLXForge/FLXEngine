#pragma once

#include "MusicDefinition.h"
#include "SoundDefinition.h"
#include "../machine/MachineDefinition.h"

#include <raylib.h>
#include <cstdint>
#include <vector>

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    void init();
    void shutdown();
    void update();

    void configure(const AudioChipDefinition& audioChip);
    void play(const SoundDefinition& definition);
    void playMusic(const MusicDefinition& definition);
    void stopMusic();
    void togglePauseMusic();
    bool isMusicActive() const;
    bool isMusicPaused() const;

private:
    struct ActiveSound
    {
        Sound sound = {};
        uint64_t startedAt = 0;
        int priority = 0;
    };

    struct ActiveMusic
    {
        Sound sound = {};
        bool loaded = false;
        bool loop = false;
        bool paused = false;
    };

    Wave createWave(const SoundDefinition& definition) const;
    Wave createMusicWave(
        const MusicDefinition& definition,
        int channelCount
    ) const;
    void cleanupFinished();
    bool reserveSoundVoice();
    void unloadActiveSound(size_t index);
    bool stealMusicVoice();
    float resolveSoundFrequency(const SoundDefinition& definition) const;

    bool initialized = false;
    int sampleRate = 44100;
    uint64_t nextSoundOrder = 0;
    AudioChipDefinition chip;
    std::vector<ActiveSound> activeSounds;
    ActiveMusic activeMusic;
};
