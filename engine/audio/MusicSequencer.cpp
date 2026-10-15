#include "MusicSequencer.h"
#include "AudioSynthesis.h"
#include <utility>

MusicSequencer::MusicSequencer(const MusicDefinition& definition, int channelCount,
    const AudioChipDefinition& chip, int sampleRate)
    : chip(chip), sampleRate(std::max(1, sampleRate))
{
    channelCount = std::clamp(channelCount, 0, static_cast<int>(definition.channels.size()));
    const float tempo =
        std::max(definition.tempo, 1.0f);

    const float quarterDuration =
        60.0f / tempo;

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

        preparedChannels.push_back(std::move(preparedChannel));
    }

    sampleCount = std::max<uint64_t>(1, static_cast<uint64_t>(duration * this->sampleRate));
    history.resize(std::max(1, static_cast<int>(static_cast<float>(this->sampleRate) * 0.045f)), 0.0f);
}

size_t MusicSequencer::generate(short* samples, size_t capacity, bool loop)
{
    size_t written = 0;
    while (written < capacity)
    {
        if (finished())
        {
            if (!loop)
            {
                break;
            }
            cursor = 0;
            std::fill(history.begin(), history.end(), 0.0f);
        }
        const float time =
            static_cast<float>(cursor) / static_cast<float>(sampleRate);

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
            (cursor >= history.size() ? history[cursor % history.size()] * maxEcho * 0.35f : 0.0f);

        mixedValue =
            applyFidelity(mixedValue, chip);

        history[cursor % history.size()] =
            mixedValue;

        samples[written] =
            static_cast<short>(
                mixedValue * 32000.0f
            );
        ++cursor;
        ++written;
    }
    return written;
}
