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

    std::string waveForSource(
        const AudioSourceDefinition& source
    )
    {
        if (source.type == "noise")
        {
            return "noise";
        }

        return source.wave;
    }

    AudioSourceDefinition sourceForChip(
        AudioSourceDefinition source,
        const AudioChipDefinition& chip
    )
    {
        if (source.wave == "square")
        {
            source.duty = 0.5f;
        }

        if (
            chip.synthesisNoise == "none" &&
            (
                source.type == "noise" ||
                source.wave == "noise"
            )
        )
        {
            source.type = "oscillator";
            source.wave = "square";
        }

        if (chip.synthesisModel == "pulse")
        {
            if (source.wave == "sine")
            {
                source.wave = "triangle";
            }
            else if (source.wave == "saw")
            {
                source.wave = "pulse";
            }
        }

        source.duty =
            std::clamp(source.duty, 0.05f, 0.95f);

        return source;
    }

    float sampleValue(
        const AudioSourceDefinition& source,
        float frequency,
        float time
    )
    {
        const std::string wave =
            waveForSource(source);

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

        if (wave == "pulse")
        {
            return phase < source.duty ? 1.0f : -1.0f;
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

    float envelopeValue(
        const AudioEnvelopeDefinition& envelope,
        float time,
        float duration
    )
    {
        float attack =
            std::max(0.0f, envelope.attack);
        float decay =
            std::max(0.0f, envelope.decay);
        const float sustain =
            std::clamp(envelope.sustain, 0.0f, 1.0f);
        float release =
            std::max(0.0f, envelope.release);

        const float segmentTotal =
            attack + decay + release;

        if (segmentTotal > duration && segmentTotal > 0.0f)
        {
            const float scale =
                duration / segmentTotal;

            attack *= scale;
            decay *= scale;
            release *= scale;
        }

        if (attack > 0.0f && time <= attack)
        {
            return std::clamp(time / attack, 0.0f, 1.0f);
        }

        const float decayStart =
            attack;
        const float decayEnd =
            attack + decay;

        if (decay > 0.0f && time <= decayEnd)
        {
            const float progress =
                std::clamp((time - decayStart) / decay, 0.0f, 1.0f);

            return 1.0f + (sustain - 1.0f) * progress;
        }

        if (release > 0.0f && time > duration - release)
        {
            const float progress =
                std::clamp((time - (duration - release)) / release, 0.0f, 1.0f);

            return sustain * (1.0f - progress);
        }

        return decay > 0.0f ? sustain : 1.0f;
    }

    float movementStrength(
        const AudioChipDefinition& chip
    )
    {
        if (chip.synthesisMovement == "none")
        {
            return 0.0f;
        }

        if (chip.synthesisMovement == "simple")
        {
            return 0.5f;
        }

        return 1.0f;
    }

    float textureNoiseMultiplier(
        const AudioChipDefinition& chip
    )
    {
        if (chip.synthesisTexture == "clean")
        {
            return 0.35f;
        }

        if (chip.synthesisTexture == "rough" || chip.synthesisTexture == "coarse")
        {
            return 1.6f;
        }

        if (chip.synthesisTexture == "raw")
        {
            return 2.0f;
        }

        return 1.0f;
    }

    float noiseCapabilityMultiplier(
        const AudioChipDefinition& chip
    )
    {
        if (chip.synthesisNoise == "none")
        {
            return 0.0f;
        }

        if (chip.synthesisNoise == "simple")
        {
            return 0.6f;
        }

        return 1.0f;
    }

    float quantizeValue(
        float value,
        int levels
    )
    {
        if (levels <= 1)
        {
            return value;
        }

        const float normalized =
            (std::clamp(value, -1.0f, 1.0f) + 1.0f) * 0.5f;

        const float quantized =
            std::round(normalized * static_cast<float>(levels - 1)) /
            static_cast<float>(levels - 1);

        return quantized * 2.0f - 1.0f;
    }

    float applyFidelity(
        float value,
        const AudioChipDefinition& chip
    )
    {
        if (chip.fidelityDynamics == "limited")
        {
            value =
                std::tanh(value * 1.8f) / std::tanh(1.8f);
        }
        else if (chip.fidelityDynamics == "fixed")
        {
            value =
                value >= 0.0f ? 0.75f : -0.75f;
        }

        if (chip.fidelityResolution == "very_low")
        {
            value = quantizeValue(value, 8);
        }
        else if (chip.fidelityResolution == "low")
        {
            value = quantizeValue(value, 16);
        }
        else if (chip.fidelityResolution == "medium")
        {
            value = quantizeValue(value, 64);
        }

        return std::clamp(value, -1.0f, 1.0f);
    }

    float applyEcho(
        const std::vector<float>& samples,
        int index,
        int delaySamples,
        float echo
    )
    {
        if (echo <= 0.0f || index < delaySamples)
        {
            return 0.0f;
        }

        return samples[static_cast<size_t>(index - delaySamples)] *
            std::clamp(echo, 0.0f, 1.0f) *
            0.35f;
    }

    float movementSlide(
        const SoundKindDefinition& kind
    )
    {
        if (kind.slide != 0.0f)
        {
            return kind.slide;
        }

        if (kind.movement.type == "fall")
        {
            return -kind.noteFrequency * std::clamp(kind.movement.amount, 0.0f, 1.0f);
        }

        if (kind.movement.type == "rise")
        {
            return kind.noteFrequency * std::clamp(kind.movement.amount, 0.0f, 1.0f);
        }

        return 0.0f;
    }

    float modulatedFrequency(
        float baseFrequency,
        float time,
        float progress,
        const SoundKindDefinition& kind,
        const AudioChipDefinition& chip
    )
    {
        const float slide =
            movementSlide(kind);

        float frequency =
            baseFrequency + slide * progress;

        const float amount =
            std::clamp(kind.movement.amount, 0.0f, 1.0f) *
            movementStrength(chip);

        if (kind.movement.type == "wobble")
        {
            frequency *=
                1.0f + std::sin(twoPi * 6.0f * time) * 0.04f * amount;
        }

        return std::max(1.0f, frequency);
    }

    float movementVolume(
        const AudioMovementDefinition& movement,
        float time,
        const AudioChipDefinition& chip
    )
    {
        const float amount =
            std::clamp(movement.amount, 0.0f, 1.0f) *
            movementStrength(chip);

        if (movement.type == "pulse")
        {
            const float lfo =
                0.5f + 0.5f * std::sin(twoPi * 8.0f * time);

            return 1.0f - lfo * 0.65f * amount;
        }

        return 1.0f;
    }

    float applyMaterial(
        float value,
        const AudioSourceDefinition& source,
        float frequency,
        float time,
        const AudioMaterialDefinition& material,
        const AudioChipDefinition& chip
    )
    {
        const float brightness =
            std::clamp(material.brightness, 0.0f, 1.0f);

        const float secondHarmonic =
            sampleValue(source, frequency * 2.0f, time);

        value =
            value * (0.9f - 0.2f * brightness) +
            secondHarmonic * (0.05f + 0.25f * brightness);

        if (material.resonance > 0.0f)
        {
            value +=
                std::sin(twoPi * frequency * 2.0f * time) *
                0.18f *
                std::clamp(material.resonance, 0.0f, 1.0f);
        }

        if (material.metal > 0.0f)
        {
            value +=
                std::sin(twoPi * frequency * 1.4142f * time) *
                0.22f *
                std::clamp(material.metal, 0.0f, 1.0f);
        }

        float roughness =
            std::clamp(material.roughness, 0.0f, 1.0f);

        if (chip.synthesisTexture == "clean")
        {
            roughness *= 0.35f;
        }
        else if (chip.synthesisTexture == "rough" || chip.synthesisTexture == "coarse")
        {
            roughness *= 1.5f;
        }
        else if (chip.synthesisTexture == "raw")
        {
            roughness *= 2.0f;
        }

        if (roughness > 0.0f)
        {
            const float instability =
                std::sin(twoPi * 37.0f * time) *
                0.08f *
                std::clamp(roughness, 0.0f, 1.0f);

            value *= 1.0f + instability;
        }

        const float noiseAmount =
            std::clamp(
                material.noise *
                textureNoiseMultiplier(chip) *
                noiseCapabilityMultiplier(chip),
                0.0f,
                1.0f
            );

        if (noiseAmount > 0.0f)
        {
            const float noise =
                static_cast<float>(GetRandomValue(-1000, 1000)) / 1000.0f;

            value =
                value * (1.0f - noiseAmount) +
                noise * noiseAmount;
        }

        return value;
    }
    bool noteInRange(
        float frequency,
        const InstrumentRangeDefinition& range
    )
    {
        float minFrequency = 0.0f;
        float maxFrequency = 0.0f;

        if (!NoteTools::noteToFrequency(range.min, minFrequency))
        {
            minFrequency = 16.35f;
        }

        if (!NoteTools::noteToFrequency(range.max, maxFrequency))
        {
            maxFrequency = 7902.13f;
        }

        if (minFrequency > maxFrequency)
        {
            std::swap(minFrequency, maxFrequency);
        }

        return frequency >= minFrequency && frequency <= maxFrequency;
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

    StopSound(activeSounds[index].sound);
    UnloadSound(activeSounds[index].sound);

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
        chip.voicesOverflow == "steal_from_music" ||
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
        chip.voicesOverflow == "ignore" ||
        chip.voicesOverflow == "replace_newest"
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
        bool replaceMusic =
            activeMusicVoiceUse() > 0;

        uint64_t oldestOrder =
            replaceMusic
            ? activeMusic.startedAt
            : std::numeric_limits<uint64_t>::max();

        size_t selectedIndex = 0;

        for (size_t i = 0; i < activeSounds.size(); ++i)
        {
            if (activeSounds[i].startedAt < oldestOrder)
            {
                replaceMusic = false;
                selectedIndex = i;
                oldestOrder = activeSounds[i].startedAt;
            }
        }

        if (replaceMusic)
        {
            stopActiveMusic();
        }
        else if (!activeSounds.empty())
        {
            unloadActiveSound(selectedIndex);
        }

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

        unloadActiveSound(selectedIndex);
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
    if (
        !activeMusic.loaded ||
        activeMusic.paused ||
        !IsSoundPlaying(activeMusic.sound)
    )
    {
        return false;
    }

    PauseSound(activeMusic.sound);
    activeMusic.paused = true;
    activeMusic.pausedBySound = true;

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

    StopSound(activeMusic.sound);
    UnloadSound(activeMusic.sound);

    activeMusic = ActiveMusic{};
}

void AudioSystem::resumeMusicAfterSoundSteal()
{
    if (
        activeMusic.loaded &&
        activeMusic.pausedBySound &&
        activeSounds.empty()
    )
    {
        ResumeSound(activeMusic.sound);
        activeMusic.paused = false;
        activeMusic.pausedBySound = false;

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
    if (
        !activeMusic.loaded ||
        activeMusic.paused ||
        !IsSoundPlaying(activeMusic.sound)
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

    Wave wave =
        createMusicWave(definition, channelCount);

    activeMusic.sound =
        LoadSoundFromWave(wave);
    activeMusic.loaded = true;
    activeMusic.loop = definition.loop;
    activeMusic.paused = false;
    activeMusic.pausedBySound = false;
    activeMusic.startedAt = nextSoundOrder++;
    activeMusic.voiceCount = channelCount;

    PlaySound(activeMusic.sound);

    UnloadWave(wave);
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
        AudioSourceDefinition source;
        AudioToneDefinition tone;
        InstrumentPlayDefinition play;
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
        preparedChannel.source = sourceForChip(channel.instrument.source, chip);
        preparedChannel.tone = channel.instrument.tone;
        preparedChannel.play = channel.instrument.play;
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
                if (
                    channel.instrument.source.type == "noise" ||
                    channel.instrument.source.wave == "noise"
                )
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

            if (
                !noteInRange(
                    preparedNote.frequency,
                    channel.instrument.range
                )
            )
            {
                Logger::warning(
                    "audio",
                    "Music note '" + note +
                    "' outside instrument range in channel: " + channel.id
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

    std::vector<float> generated(
        static_cast<size_t>(sampleCount),
        0.0f
    );

    const int echoDelaySamples =
        std::max(1, static_cast<int>(static_cast<float>(sampleRate) * 0.045f));

    for (int i = 0; i < sampleCount; ++i)
    {
        const float time =
            static_cast<float>(i) / static_cast<float>(sampleRate);

        float mixedValue = 0.0f;
        float maxEcho = 0.0f;

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

            float frequency =
                note.frequency;

            if (channel.play.vibrato > 0.0f)
            {
                frequency *=
                    1.0f +
                    std::sin(twoPi * 6.0f * noteTime) *
                    0.04f *
                    std::clamp(channel.play.vibrato, 0.0f, 1.0f) *
                    movementStrength(chip);
            }

            float value =
                sampleValue(channel.source, frequency, noteTime);

            value =
                applyMaterial(
                    value,
                    channel.source,
                    frequency,
                    noteTime,
                    channel.tone.material,
                    chip
                );

            value *=
                envelopeValue(
                    channel.tone.envelope,
                    noteTime,
                    channel.stepDuration
                );

            mixedValue +=
                value * channel.volume;

            maxEcho =
                std::max(
                    maxEcho,
                    std::clamp(channel.tone.space.echo, 0.0f, 1.0f)
                );
        }

        mixedValue +=
            applyEcho(
                generated,
                i,
                echoDelaySamples,
                maxEcho
            );

        mixedValue =
            applyFidelity(mixedValue, chip);

        generated[static_cast<size_t>(i)] =
            mixedValue;

        samples[static_cast<size_t>(i)] =
            static_cast<short>(
                mixedValue * 32000.0f
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
