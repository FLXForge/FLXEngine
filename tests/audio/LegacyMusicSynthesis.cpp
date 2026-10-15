// Frozen whole-score reference algorithm used only by test targets.
#include "../../engine/audio/AudioSystem.h"
#include "../../engine/audio/AudioSynthesis.h"

Wave AudioSystem::testCreateMusicWave(
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
