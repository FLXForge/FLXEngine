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
        double startedTime = 0.0;
        double duration = 0.0;
        int priority = 0;
    };

    struct ActiveMusic
    {
        Sound sound = {};
        bool loaded = false;
        bool loop = false;
        bool paused = false;
        bool pausedBySound = false;
        uint64_t startedAt = 0;
        int voiceCount = 0;
    };

    Wave createWave(const SoundDefinition& definition) const;
    Wave createMusicWave(
        const MusicDefinition& definition,
        int channelCount
    ) const;
    void cleanupFinished();
    bool reserveSoundVoice();
    bool reserveReservedSoundVoice();
    bool reserveSharedSoundVoice();
    void unloadActiveSound(size_t index);
    bool stealMusicVoice();
    void stopActiveMusic();
    void resumeMusicAfterSoundSteal();
    bool voicesAreShared() const;
    int sharedVoiceCount() const;
    int activeMusicVoiceUse() const;

    bool initialized = false;
    int sampleRate = 44100;
    uint64_t nextSoundOrder = 0;
    AudioChipDefinition chip;
    std::vector<ActiveSound> activeSounds;
    ActiveMusic activeMusic;
};
