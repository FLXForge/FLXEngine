#include "AudioSystem.h"
#include "NoteTools.h"
#include "../debug/Logger.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace
{
    constexpr float twoPi = 6.28318530718f;

    float sampleValue(
        const std::string& wave,
        float frequency,
        float time
    )
    {
        const float phase =
            std::fmod(time * frequency, 1.0f);

        if (wave == "sine")
        {
            return std::sin(twoPi * phase);
        }

        if (wave == "triangle")
        {
            return 1.0f - 4.0f * std::fabs(phase - 0.5f);
        }

        if (wave == "saw")
        {
            return 2.0f * phase - 1.0f;
        }

        if (wave == "noise")
        {
            return static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f;
        }

        return phase < 0.5f ? 1.0f : -1.0f;
    }

    bool noteIsRest(const std::string& note)
    {
        return note == "-" || note.empty();
    }

    float lengthToQuarterMultiplier(const std::string& length)
    {
        if (length == "1/1")
        {
            return 4.0f;
        }

        if (length == "1/2")
        {
            return 2.0f;
        }

        if (length == "1/8")
        {
            return 0.5f;
        }

        if (length == "1/16")
        {
            return 0.25f;
        }

        return 1.0f;
    }
}

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
    initialized = true;
}

void AudioSystem::shutdown()
{
    if (!initialized)
    {
        return;
    }

    for (auto& activeSound : activeSounds)
    {
        UnloadSound(activeSound.sound);
    }

    activeSounds.clear();
    stopMusic();

    CloseAudioDevice();
    initialized = false;
}

void AudioSystem::update()
{
    if (!initialized)
    {
        return;
    }

    cleanupFinished();

    if (
        activeMusic.loaded &&
        !activeMusic.paused &&
        !IsSoundPlaying(activeMusic.sound)
    )
    {
        if (activeMusic.loop)
        {
            PlaySound(activeMusic.sound);
        }
        else
        {
            stopMusic();
        }
    }
}

void AudioSystem::configure(const AudioChipDefinition& audioChip)
{
    chip =
        audioChip;
}

void AudioSystem::cleanupFinished()
{
    activeSounds.erase(
        std::remove_if(
            activeSounds.begin(),
            activeSounds.end(),
            [](ActiveSound& activeSound)
            {
                if (IsSoundPlaying(activeSound.sound))
                {
                    return false;
                }

                UnloadSound(activeSound.sound);
                return true;
            }
        ),
        activeSounds.end()
    );
}

void AudioSystem::unloadActiveSound(size_t index)
{
    if (index >= activeSounds.size())
    {
        return;
    }

    StopSound(activeSounds[index].sound);
    UnloadSound(activeSounds[index].sound);

    activeSounds.erase(
        activeSounds.begin() + static_cast<std::ptrdiff_t>(index)
    );
}

bool AudioSystem::reserveSoundVoice()
{
    cleanupFinished();

    const int soundVoices =
        std::max(0, chip.voicesSound);

    if (soundVoices == 0)
    {
        if (chip.voicesOverflow == "steal_from_music" && stealMusicVoice())
        {
            return true;
        }

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

    if (chip.voicesOverflow == "steal_from_music" && stealMusicVoice())
    {
        return true;
    }

    if (
        chip.voicesOverflow == "ignore" ||
        chip.voicesOverflow == "replace_newest"
    )
    {
        Logger::debug(
            "audio",
            "Sound ignored because sound voices are full"
        );

        return false;
    }

    size_t selectedIndex = 0;

    if (chip.voicesOverflow == "replace_lowest_priority")
    {
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
    }
    else
    {
        uint64_t oldestOrder =
            std::numeric_limits<uint64_t>::max();

        for (size_t i = 0; i < activeSounds.size(); ++i)
        {
            if (activeSounds[i].startedAt < oldestOrder)
            {
                selectedIndex = i;
                oldestOrder = activeSounds[i].startedAt;
            }
        }
    }

    unloadActiveSound(selectedIndex);

    return true;
}

bool AudioSystem::stealMusicVoice()
{
    if (!activeMusic.loaded)
    {
        Logger::debug(
            "audio",
            "Sound ignored because there is no music voice to steal"
        );

        return false;
    }

    stopMusic();

    Logger::debug(
        "audio",
        "Sound stole a music voice"
    );

    return true;
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
            0
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

    const int musicVoices =
        std::max(0, chip.voicesMusic);

    if (musicVoices == 0)
    {
        Logger::warning(
            "audio",
            "Music ignored because audio.voices.music is 0"
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

    Wave wave =
        createMusicWave(definition, channelCount);

    activeMusic.sound =
        LoadSoundFromWave(wave);
    activeMusic.loaded = true;
    activeMusic.loop = definition.loop;
    activeMusic.paused = false;

    PlaySound(activeMusic.sound);

    UnloadWave(wave);
}

void AudioSystem::stopMusic()
{
    if (!activeMusic.loaded)
    {
        return;
    }

    StopSound(activeMusic.sound);
    UnloadSound(activeMusic.sound);

    activeMusic = ActiveMusic{};
}

void AudioSystem::togglePauseMusic()
{
    if (!activeMusic.loaded)
    {
        return;
    }

    if (activeMusic.paused)
    {
        ResumeSound(activeMusic.sound);
        activeMusic.paused = false;
    }
    else
    {
        PauseSound(activeMusic.sound);
        activeMusic.paused = true;
    }
}

bool AudioSystem::isMusicActive() const
{
    return
        activeMusic.loaded &&
        (
            activeMusic.paused ||
            IsSoundPlaying(activeMusic.sound)
        );
}

bool AudioSystem::isMusicPaused() const
{
    return activeMusic.loaded && activeMusic.paused;
}

float AudioSystem::resolveSoundFrequency(
    const SoundDefinition& definition
) const
{
    if (definition.hasFrequency)
    {
        return definition.frequency;
    }

    if (!definition.note.empty())
    {
        float frequency = 0.0f;

        if (NoteTools::noteToFrequency(definition.note, frequency))
        {
            return frequency;
        }

        Logger::warning(
            "audio",
            "Invalid sound note: " + definition.note
        );
    }

    return definition.frequency;
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

    const float frequency =
        resolveSoundFrequency(definition);

    for (int i = 0; i < sampleCount; ++i)
    {
        const float time =
            static_cast<float>(i) / static_cast<float>(sampleRate);

        const float value =
            sampleValue(
                definition.wave,
                frequency,
                time
            );

        samples[static_cast<size_t>(i)] =
            static_cast<short>(
                std::clamp(value, -1.0f, 1.0f) * 32000.0f
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

Wave AudioSystem::createMusicWave(
    const MusicDefinition& definition,
    int channelCount
) const
{
    struct PreparedNote
    {
        bool rest = true;
        float frequency = 440.0f;
    };

    struct PreparedChannel
    {
        std::string id;
        std::string wave;
        float volume = 1.0f;
        float stepDuration = 0.5f;
        std::vector<PreparedNote> notes;
    };

    const float tempo =
        std::max(definition.tempo, 1.0f);

    const float quarterDuration =
        60.0f / tempo;

    std::vector<PreparedChannel> preparedChannels;
    preparedChannels.reserve(static_cast<size_t>(channelCount));

    float duration = 0.01f;

    for (int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
    {
        const MusicChannelDefinition& channel =
            definition.channels[static_cast<size_t>(channelIndex)];

        PreparedChannel preparedChannel;
        preparedChannel.id = channel.id;
        preparedChannel.wave = channel.wave;
        preparedChannel.volume =
            std::clamp(channel.volume, 0.0f, 1.0f);
        preparedChannel.stepDuration =
            quarterDuration * lengthToQuarterMultiplier(channel.length);
        preparedChannel.notes.reserve(channel.notes.size());

        for (const std::string& note : channel.notes)
        {
            PreparedNote preparedNote;

            if (noteIsRest(note))
            {
                preparedChannel.notes.push_back(preparedNote);
                continue;
            }

            if (note == "x" || note == "X")
            {
                if (channel.wave == "noise")
                {
                    preparedNote.rest = false;
                }
                else
                {
                    Logger::warning(
                        "audio",
                        "Music note 'x' ignored in non-noise channel: " +
                        channel.id
                    );
                }

                preparedChannel.notes.push_back(preparedNote);
                continue;
            }

            if (!NoteTools::noteToFrequency(note, preparedNote.frequency))
            {
                Logger::warning(
                    "audio",
                    "Invalid music note '" + note +
                    "' in channel: " + channel.id
                );

                preparedChannel.notes.push_back(preparedNote);
                continue;
            }

            preparedNote.rest = false;
            preparedChannel.notes.push_back(preparedNote);
        }

        duration =
            std::max(
                duration,
                preparedChannel.stepDuration *
                    static_cast<float>(preparedChannel.notes.size())
            );

        preparedChannels.push_back(preparedChannel);
    }

    const int sampleCount =
        std::max(1, static_cast<int>(duration * sampleRate));

    std::vector<short> samples(
        static_cast<size_t>(sampleCount)
    );

    for (int i = 0; i < sampleCount; ++i)
    {
        const float time =
            static_cast<float>(i) / static_cast<float>(sampleRate);

        float mixedValue = 0.0f;

        for (const PreparedChannel& channel : preparedChannels)
        {
            const size_t step =
                static_cast<size_t>(time / channel.stepDuration);

            if (step >= channel.notes.size())
            {
                continue;
            }

            const PreparedNote& note =
                channel.notes[step];

            if (note.rest)
            {
                continue;
            }

            const float noteTime =
                time - static_cast<float>(step) * channel.stepDuration;

            mixedValue +=
                sampleValue(channel.wave, note.frequency, noteTime) *
                channel.volume;
        }

        samples[static_cast<size_t>(i)] =
            static_cast<short>(
                std::clamp(mixedValue, -1.0f, 1.0f) * 32000.0f
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
