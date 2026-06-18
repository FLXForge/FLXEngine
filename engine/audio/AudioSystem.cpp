#include "AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    constexpr float twoPi = 6.28318530718f;

    float sampleValue(
        const SoundDefinition& definition,
        float time
    )
    {
        const float phase =
            std::fmod(time * definition.frequency, 1.0f);

        if (definition.wave == "sine")
        {
            return std::sin(twoPi * phase);
        }

        if (definition.wave == "triangle")
        {
            return 1.0f - 4.0f * std::fabs(phase - 0.5f);
        }

        if (definition.wave == "saw")
        {
            return 2.0f * phase - 1.0f;
        }

        if (definition.wave == "noise")
        {
            return static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f;
        }

        return phase < 0.5f ? 1.0f : -1.0f;
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

    for (auto& sound : activeSounds)
    {
        UnloadSound(sound);
    }

    activeSounds.clear();

    CloseAudioDevice();
    initialized = false;
}

void AudioSystem::update()
{
    if (!initialized)
    {
        return;
    }

    activeSounds.erase(
        std::remove_if(
            activeSounds.begin(),
            activeSounds.end(),
            [](Sound& sound)
            {
                if (IsSoundPlaying(sound))
                {
                    return false;
                }

                UnloadSound(sound);
                return true;
            }
        ),
        activeSounds.end()
    );
}

void AudioSystem::play(const SoundDefinition& definition)
{
    if (!initialized)
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

    activeSounds.push_back(sound);
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

    for (int i = 0; i < sampleCount; ++i)
    {
        const float time =
            static_cast<float>(i) / static_cast<float>(sampleRate);

        const float value =
            sampleValue(definition, time);

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
