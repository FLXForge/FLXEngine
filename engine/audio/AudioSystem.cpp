#include "AudioSystem.h"
#include "AudioSynthesis.h"
#include "NoteTools.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem()
{
    shutdown();
}

void AudioSystem::init()
{
    if (initialized)
    {
        return;
    }

    InitAudioDevice();
    initialized = IsAudioDeviceReady();
    ownsDevice = initialized;
}

bool AudioSystem::attachDevice()
{
    if (initialized)
    {
        return !ownsDevice;
    }
    initialized = IsAudioDeviceReady();
    ownsDevice = false;
    return initialized;
}

void AudioSystem::shutdown()
{
    for (auto& activeSound : activeSounds)
    {
        if (activeSound.loaded)
        {
            UnloadSound(activeSound.sound);
        }
    }
    activeSounds.clear();
    stopMusic();
    if (initialized && ownsDevice)
    {
        CloseAudioDevice();
    }
    initialized = false;
    ownsDevice = false;
    nextSoundOrder = 0;
}

void AudioSystem::update()
{
    if (!initialized)
    {
        return;
    }

    cleanupFinished();

    if (activeMusic.loaded && !activeMusic.paused && !activeMusic.suspendedBySound)
    {
        // Each successful refill retires the oldest queued half-buffer.
        // Silence markers let us drain both halves using raylib's public OR query.
        for (int refill = 0; refill < 2 && IsAudioStreamProcessed(activeMusic.stream); ++refill)
        {
            activeMusic.pending[0] = activeMusic.pending[1];
            activeMusic.pending[1] = false;
            if (activeMusic.sequencer->finished() && !activeMusic.loop &&
                !activeMusic.pending[0])
            {
                stopMusic();
                break;
            }
            feedMusicStream();
        }
    }
}

void AudioSystem::configure(const AudioChipDefinition& audioChip)
{
    chip =
        audioChip;
}

#ifdef FLX_TESTING
bool AudioSystem::testPlayLogicalSound(const SoundDefinition& definition)
{
    if (!reserveSoundVoice())
    {
        return false;
    }

    activeSounds.push_back(
        ActiveSound{
            Sound{},
            nextSoundOrder++,
            0.0,
            static_cast<double>(definition.duration),
            definition.priority,
            false
        }
    );

    return true;
}

bool AudioSystem::testPlayLogicalSound(int priority)
{
    SoundDefinition definition;
    definition.priority = priority;
    definition.duration = 1000.0f;

    return testPlayLogicalSound(definition);
}

void AudioSystem::testStartLogicalMusic(int voiceCount)
{
    activeMusic = ActiveMusic{};
    activeMusic.loaded = true;
    activeMusic.paused = false;
    activeMusic.suspendedBySound = false;
    activeMusic.startedAt = nextSoundOrder++;
    activeMusic.voiceCount = std::max(1, voiceCount);
    activeMusic.synthetic = true;
}

void AudioSystem::testFinishAllSounds()
{
    activeSounds.clear();
    resumeMusicAfterSoundSteal();
}

size_t AudioSystem::testActiveSoundCount() const
{
    return activeSounds.size();
}

std::vector<int> AudioSystem::testActiveSoundPriorities() const
{
    std::vector<int> priorities;
    priorities.reserve(activeSounds.size());

    for (const ActiveSound& activeSound : activeSounds)
    {
        priorities.push_back(activeSound.priority);
    }

    return priorities;
}

std::vector<uint64_t> AudioSystem::testActiveSoundOrders() const
{
    std::vector<uint64_t> orders;
    orders.reserve(activeSounds.size());

    for (const ActiveSound& activeSound : activeSounds)
    {
        orders.push_back(activeSound.startedAt);
    }

    return orders;
}

bool AudioSystem::testMusicLoaded() const
{
    return activeMusic.loaded;
}

bool AudioSystem::testMusicPaused() const
{
    return activeMusic.paused;
}

bool AudioSystem::testMusicSuspendedBySound() const
{
    return activeMusic.suspendedBySound;
}
#endif

void AudioSystem::cleanupFinished()
{
    const double now =
        GetTime();

    activeSounds.erase(
        std::remove_if(
            activeSounds.begin(),
            activeSounds.end(),
            [now](ActiveSound& activeSound)
            {
                const bool durationFinished =
                    activeSound.duration > 0.0 &&
                    now >= activeSound.startedTime + activeSound.duration;

                if (!activeSound.loaded)
                {
                    return durationFinished;
                }

                if (!durationFinished && IsSoundPlaying(activeSound.sound))
                {
                    return false;
                }

                UnloadSound(activeSound.sound);
                return true;
            }
        ),
        activeSounds.end()
    );

    resumeMusicAfterSoundSteal();
}

void AudioSystem::unloadActiveSound(size_t index)
{
    if (index >= activeSounds.size())
    {
        return;
    }

    if (activeSounds[index].loaded)
    {
        StopSound(activeSounds[index].sound);
        UnloadSound(activeSounds[index].sound);
    }

    activeSounds.erase(
        activeSounds.begin() + static_cast<std::ptrdiff_t>(index)
    );

    resumeMusicAfterSoundSteal();
}

bool AudioSystem::reserveSoundVoice()
{
    if (voicesAreShared())
    {
        return reserveSharedSoundVoice();
    }

    return reserveReservedSoundVoice();
}

size_t AudioSystem::selectOldestSound() const
{
    size_t selectedIndex = 0;
    uint64_t selectedOrder =
        std::numeric_limits<uint64_t>::max();

    for (size_t i = 0; i < activeSounds.size(); ++i)
    {
        if (activeSounds[i].startedAt < selectedOrder)
        {
            selectedIndex = i;
            selectedOrder = activeSounds[i].startedAt;
        }
    }

    return selectedIndex;
}

size_t AudioSystem::selectNewestSound() const
{
    size_t selectedIndex = 0;
    uint64_t selectedOrder = 0;

    for (size_t i = 0; i < activeSounds.size(); ++i)
    {
        if (activeSounds[i].startedAt >= selectedOrder)
        {
            selectedIndex = i;
            selectedOrder = activeSounds[i].startedAt;
        }
    }

    return selectedIndex;
}

size_t AudioSystem::selectLowestPrioritySound() const
{
    size_t selectedIndex = 0;
    int selectedPriority =
        std::numeric_limits<int>::max();

    uint64_t selectedOrder =
        std::numeric_limits<uint64_t>::max();

    for (size_t i = 0; i < activeSounds.size(); ++i)
    {
        const ActiveSound& activeSound =
            activeSounds[i];

        if (
            activeSound.priority < selectedPriority ||
            (
                activeSound.priority == selectedPriority &&
                activeSound.startedAt < selectedOrder
            )
        )
        {
            selectedIndex = i;
            selectedPriority = activeSound.priority;
            selectedOrder = activeSound.startedAt;
        }
    }

    return selectedIndex;
}

bool AudioSystem::reserveReservedSoundVoice()
{
    cleanupFinished();

    const int soundVoices =
        std::max(0, chip.voicesSound);

    if (soundVoices == 0)
    {
        Logger::debug(
            "audio",
            "Sound ignored because audio.voices.sound is 0"
        );

        return false;
    }

    if (activeSounds.size() < static_cast<size_t>(soundVoices))
    {
        return true;
    }

    if (
        chip.voicesOverflow == "ignore" ||
        chip.voicesOverflow == "steal_from_music"
    )
    {
        Logger::debug(
            "audio",
            "Sound ignored because sound voices are full"
        );

        return false;
    }

    if (chip.voicesOverflow == "replace_lowest_priority")
    {
        unloadActiveSound(selectLowestPrioritySound());
    }
    else if (chip.voicesOverflow == "replace_newest")
    {
        unloadActiveSound(selectNewestSound());
    }
    else
    {
        unloadActiveSound(selectOldestSound());
    }

    return true;
}

bool AudioSystem::reserveSharedSoundVoice()
{
    cleanupFinished();

    const int voiceCount =
        sharedVoiceCount();

    if (voiceCount == 0)
    {
        Logger::debug(
            "audio",
            "Sound ignored because shared audio voice count is 0"
        );

        return false;
    }

    const int activeVoices =
        static_cast<int>(activeSounds.size()) +
        activeMusicVoiceUse();

    if (activeVoices < voiceCount)
    {
        return true;
    }

    if (chip.voicesOverflow == "steal_from_music")
    {
        if (stealMusicVoice())
        {
            return true;
        }

        Logger::debug(
            "audio",
            "Sound ignored because shared voices are full and no music voice can be stolen"
        );

        return false;
    }

    if (
        chip.voicesOverflow == "ignore"
    )
    {
        Logger::debug(
            "audio",
            "Sound ignored because shared voices are full"
        );

        return false;
    }

    if (chip.voicesOverflow == "replace_oldest")
    {
        if (activeSounds.empty())
        {
            Logger::debug(
                "audio",
                "Sound ignored because shared voices are full"
            );

            return false;
        }

        unloadActiveSound(selectOldestSound());
        return true;
    }

    if (chip.voicesOverflow == "replace_newest")
    {
        if (activeSounds.empty())
        {
            Logger::debug(
                "audio",
                "Sound ignored because shared voices are full"
            );

            return false;
        }

        unloadActiveSound(selectNewestSound());
        return true;
    }

    if (chip.voicesOverflow == "replace_lowest_priority")
    {
        if (activeSounds.empty())
        {
            Logger::debug(
                "audio",
                "Sound ignored because shared voices are full"
            );

            return false;
        }

        unloadActiveSound(selectLowestPrioritySound());
        return true;
    }

    Logger::debug(
        "audio",
        "Sound ignored because shared voices are full"
    );

    return false;
}

bool AudioSystem::stealMusicVoice()
{
#ifdef FLX_TESTING
    if (
        activeMusic.loaded &&
        activeMusic.synthetic &&
        !activeMusic.paused &&
        !activeMusic.suspendedBySound
    )
    {
        activeMusic.suspendedBySound = true;

        Logger::debug(
            "audio",
            "Sound stole a shared music voice"
        );

        return true;
    }
#endif

    if (
        !activeMusic.loaded ||
        activeMusic.paused ||
        activeMusic.suspendedBySound ||
        !IsAudioStreamPlaying(activeMusic.stream)
    )
    {
        return false;
    }

    PauseAudioStream(activeMusic.stream);
    activeMusic.suspendedBySound = true;

    Logger::debug(
        "audio",
        "Sound stole a shared music voice"
    );

    return true;
}

void AudioSystem::stopActiveMusic()
{
    if (!activeMusic.loaded)
    {
        return;
    }

#ifdef FLX_TESTING
    if (activeMusic.synthetic)
    {
        activeMusic = ActiveMusic{};
        return;
    }
#endif

    StopAudioStream(activeMusic.stream);
    UnloadAudioStream(activeMusic.stream);

    activeMusic = ActiveMusic{};
}

void AudioSystem::resumeMusicAfterSoundSteal()
{
    if (
        activeMusic.loaded &&
        activeMusic.suspendedBySound &&
        activeSounds.empty()
    )
    {
        activeMusic.suspendedBySound = false;

        if (activeMusic.paused)
        {
            return;
        }

#ifdef FLX_TESTING
        if (!activeMusic.synthetic)
        {
            ResumeAudioStream(activeMusic.stream);
        }
#else
        ResumeAudioStream(activeMusic.stream);
#endif

        Logger::debug(
            "audio",
            "Music resumed after shared voice release"
        );
    }
}

bool AudioSystem::voicesAreShared() const
{
    return chip.voicesMode == "shared";
}

int AudioSystem::sharedVoiceCount() const
{
    return
        std::max(0, chip.voicesMusic) +
        std::max(0, chip.voicesSound);
}

int AudioSystem::activeMusicVoiceUse() const
{
#ifdef FLX_TESTING
    if (activeMusic.loaded && activeMusic.synthetic)
    {
        return activeMusic.paused || activeMusic.suspendedBySound
            ? 0
            : std::max(1, activeMusic.voiceCount);
    }
#endif

    if (
        !activeMusic.loaded ||
        activeMusic.paused ||
        activeMusic.suspendedBySound ||
        !IsAudioStreamPlaying(activeMusic.stream)
    )
    {
        return 0;
    }

    return std::max(1, activeMusic.voiceCount);
}

void AudioSystem::play(const SoundDefinition& definition)
{
    if (!initialized)
    {
        return;
    }

    if (!reserveSoundVoice())
    {
        return;
    }

    Wave wave =
        createWave(definition);

    Sound sound =
        LoadSoundFromWave(wave);

    SetSoundVolume(
        sound,
        std::clamp(definition.volume, 0.0f, 1.0f)
    );

    PlaySound(sound);

    UnloadWave(wave);

    activeSounds.push_back(
        ActiveSound{
            sound,
            nextSoundOrder++,
            GetTime(),
            static_cast<double>(std::max(definition.duration, 0.01f)),
            definition.priority,
            true
        }
    );
}

void AudioSystem::playMusic(const MusicDefinition& definition)
{
    if (!initialized)
    {
        return;
    }

    stopMusic();

    int musicVoices =
        std::max(0, chip.voicesMusic);

    if (voicesAreShared())
    {
        const int availableSharedVoices =
            sharedVoiceCount() -
            static_cast<int>(activeSounds.size());

        musicVoices =
            std::min(
                musicVoices,
                std::max(0, availableSharedVoices)
            );
    }

    if (musicVoices == 0)
    {
        Logger::warning(
            "audio",
            voicesAreShared()
            ? "Music ignored because no shared audio voices are available"
            : "Music ignored because audio.voices.music is 0"
        );

        return;
    }

    if (definition.channels.empty())
    {
        Logger::warning(
            "audio",
            "Music ignored because it has no channels"
        );

        return;
    }

    const int channelCount =
        std::min(
            musicVoices,
            static_cast<int>(definition.channels.size())
        );

    if (static_cast<int>(definition.channels.size()) > musicVoices)
    {
        Logger::warning(
            "audio",
            "Music declares more channels than available music voices; extra channels ignored"
        );
    }

    activeMusic.sequencer = std::make_unique<MusicSequencer>(definition, channelCount, chip, sampleRate);
    SetAudioStreamBufferSizeDefault(musicBlockFrames);
    activeMusic.stream = LoadAudioStream(sampleRate, 16, 1);
    SetAudioStreamBufferSizeDefault(0);
    if (!IsAudioStreamValid(activeMusic.stream))
    {
        activeMusic = ActiveMusic{};
        Logger::warning("audio", "Music stream could not be created");
        return;
    }
    activeMusic.loaded = true;
    activeMusic.loop = definition.loop;
    activeMusic.startedAt = nextSoundOrder++;
    activeMusic.voiceCount = channelCount;
    // raylib 6.0 Play resets both processed flags: prime only after Play.
    PlayAudioStream(activeMusic.stream);
    PauseAudioStream(activeMusic.stream);
    feedMusicStream();
    activeMusic.pending[0] = activeMusic.pending[1];
    feedMusicStream();
    ResumeAudioStream(activeMusic.stream);
}

void AudioSystem::feedMusicStream()
{
    const size_t count = activeMusic.sequencer->generate(
        activeMusic.block.data(), activeMusic.block.size(), activeMusic.loop);
    std::fill(activeMusic.block.begin() + count, activeMusic.block.end(), 0);
    UpdateAudioStream(activeMusic.stream, activeMusic.block.data(), musicBlockFrames);
    activeMusic.pending[1] = count != 0;
}

void AudioSystem::stopMusic()
{
    stopActiveMusic();
}

void AudioSystem::togglePauseMusic()
{
    if (!activeMusic.loaded)
    {
        return;
    }

    if (activeMusic.paused)
    {
        activeMusic.paused = false;

        if (activeMusic.suspendedBySound)
        {
            return;
        }

#ifdef FLX_TESTING
        if (!activeMusic.synthetic)
        {
            ResumeAudioStream(activeMusic.stream);
        }
#else
        ResumeAudioStream(activeMusic.stream);
#endif
    }
    else
    {
        activeMusic.paused = true;

        if (activeMusic.suspendedBySound)
        {
            return;
        }

#ifdef FLX_TESTING
        if (!activeMusic.synthetic)
        {
            PauseAudioStream(activeMusic.stream);
        }
#else
        PauseAudioStream(activeMusic.stream);
#endif
    }
}

bool AudioSystem::isMusicActive() const
{
#ifdef FLX_TESTING
    if (activeMusic.loaded && activeMusic.synthetic)
    {
        return true;
    }
#endif

    return
        activeMusic.loaded &&
        (
            activeMusic.paused ||
            activeMusic.suspendedBySound ||
            IsAudioStreamPlaying(activeMusic.stream)
        );
}

bool AudioSystem::isMusicPaused() const
{
    return activeMusic.loaded && activeMusic.paused;
}

Wave AudioSystem::createWave(const SoundDefinition& definition) const
{
    const float duration =
        std::max(definition.duration, 0.01f);

    const int sampleCount =
        std::max(1, static_cast<int>(duration * sampleRate));

    std::vector<short> samples(
        static_cast<size_t>(sampleCount)
    );

    std::vector<float> generated(
        static_cast<size_t>(sampleCount),
        0.0f
    );

    const AudioSourceDefinition source =
        sourceForChip(definition.kind.source, chip);

    const int echoDelaySamples =
        std::max(1, static_cast<int>(static_cast<float>(sampleRate) * 0.045f));

    for (int i = 0; i < sampleCount; ++i)
    {
        const float time =
            static_cast<float>(i) / static_cast<float>(sampleRate);

        const float progress =
            duration > 0.0f ? time / duration : 0.0f;

        const float frequency =
            modulatedFrequency(
                definition.kind.noteFrequency,
                time,
                progress,
                definition.kind,
                chip
            );

        float value =
            sampleValue(
                source,
                frequency,
                time
            );

        value =
            applyMaterial(
                value,
                source,
                frequency,
                time,
                definition.tone.material,
                chip
            );

        value *=
            envelopeValue(
                definition.tone.envelope,
                time,
                duration
            );

        value *=
            movementVolume(
                definition.kind.movement,
                time,
                chip
            );

        value +=
            applyEcho(
                generated,
                i,
                echoDelaySamples,
                definition.tone.space.echo
            );

        value =
            applyFidelity(value, chip);

        generated[static_cast<size_t>(i)] =
            value;

        samples[static_cast<size_t>(i)] =
            static_cast<short>(
                value * 32000.0f
            );
    }

    Wave wave{};
    wave.frameCount =
        static_cast<unsigned int>(sampleCount);
    wave.sampleRate =
        static_cast<unsigned int>(sampleRate);
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();

    Wave copy =
        WaveCopy(wave);

    return copy;
}
