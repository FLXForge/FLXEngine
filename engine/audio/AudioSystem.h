#pragma once

#include "MusicDefinition.h"
#include "MusicSequencer.h"
#include "SoundDefinition.h"
#include "../machine/MachineDefinition.h"

#include <raylib.h>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    void init();
    // Borrow an already initialized host device; shutdown releases only our resources.
    bool attachDevice();
    void shutdown();
    void update();

    void configure(const AudioChipDefinition& audioChip);
    void play(const SoundDefinition& definition);
    void playMusic(const MusicDefinition& definition);
    void stopMusic();
    void togglePauseMusic();
    bool isMusicActive() const;
    bool isMusicPaused() const;

#ifdef FLX_TESTING
    Wave testCreateMusicWave(const MusicDefinition& definition, int channelCount) const;
    bool testPlayLogicalSound(const SoundDefinition& definition);
    bool testPlayLogicalSound(int priority);
    void testStartLogicalMusic(int voiceCount);
    void testFinishAllSounds();
    size_t testActiveSoundCount() const;
    std::vector<int> testActiveSoundPriorities() const;
    std::vector<uint64_t> testActiveSoundOrders() const;
    bool testMusicLoaded() const;
    bool testMusicPaused() const;
    bool testMusicSuspendedBySound() const;
#endif

private:
    struct ActiveSound
    {
        Sound sound = {};
        uint64_t startedAt = 0;
        double startedTime = 0.0;
        double duration = 0.0;
        int priority = 0;
        bool loaded = true;
    };

    static constexpr int musicBlockFrames = 4096;

    struct ActiveMusic
    {
        AudioStream stream = {};
        std::unique_ptr<MusicSequencer> sequencer;
        std::array<short, musicBlockFrames> block{};
        std::array<bool, 2> pending{};
        bool loaded = false;
        bool loop = false;
        bool paused = false;
        bool suspendedBySound = false;
        uint64_t startedAt = 0;
        int voiceCount = 0;
#ifdef FLX_TESTING
        bool synthetic = false;
#endif
    };

    Wave createWave(const SoundDefinition& definition) const;
    void feedMusicStream();
    void cleanupFinished();
    bool reserveSoundVoice();
    bool reserveReservedSoundVoice();
    bool reserveSharedSoundVoice();
    size_t selectOldestSound() const;
    size_t selectNewestSound() const;
    size_t selectLowestPrioritySound() const;
    void unloadActiveSound(size_t index);
    bool stealMusicVoice();
    void stopActiveMusic();
    void resumeMusicAfterSoundSteal();
    bool voicesAreShared() const;
    int sharedVoiceCount() const;
    int activeMusicVoiceUse() const;

    bool initialized = false;
    bool ownsDevice = false;
    int sampleRate = 44100;
    uint64_t nextSoundOrder = 0;
    AudioChipDefinition chip;
    std::vector<ActiveSound> activeSounds;
    ActiveMusic activeMusic;
};
