#pragma once

#include "MusicDefinition.h"
#include "NoteTools.h"
#include "../machine/MachineDefinition.h"
#include "../debug/Logger.h"
#include <algorithm>
#include <cmath>
#include <raylib.h>

// Internal synthesis operations shared by effects and the music sequencer.
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
