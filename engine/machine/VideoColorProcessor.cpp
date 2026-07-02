#include "VideoColorProcessor.h"
#include "../tools/ColorParser.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    int colorDistance(
        Color left,
        Color right
    )
    {
        const int red =
            static_cast<int>(left.r) - static_cast<int>(right.r);

        const int green =
            static_cast<int>(left.g) - static_cast<int>(right.g);

        const int blue =
            static_cast<int>(left.b) - static_cast<int>(right.b);

        return red * red + green * green + blue * blue;
    }

    unsigned char quantizeChannel(
        unsigned char value,
        int levels
    )
    {
        if (levels <= 1)
        {
            return 0;
        }

        const float normalized =
            static_cast<float>(value) / 255.0f;

        const int step =
            static_cast<int>(
                std::round(normalized * static_cast<float>(levels - 1))
            );

        return static_cast<unsigned char>(
            std::clamp(
                static_cast<int>(
                    std::round(
                        static_cast<float>(step) *
                        255.0f /
                        static_cast<float>(levels - 1)
                    )
                ),
                0,
                255
            )
        );
    }

    Color closestColor(
        Color color,
        const std::vector<Color>& palette
    )
    {
        if (palette.empty())
        {
            return color;
        }

        Color best =
            palette.front();

        int bestDistance =
            colorDistance(color, best);

        for (const Color candidate : palette)
        {
            const int distance =
                colorDistance(color, candidate);

            if (distance < bestDistance)
            {
                best = candidate;
                bestDistance = distance;
            }
        }

        best.a =
            color.a;

        return best;
    }

    std::vector<Color> buildTonePalette(
        const VideoChipDefinition& video
    )
    {
        std::vector<Color> palette;

        if (!video.hasColorTone || video.colorToneLevels <= 0)
        {
            return palette;
        }

        const Color base =
            ColorParser::parse(
                video.colorToneBase,
                WHITE
            );

        if (video.colorToneLevels == 1)
        {
            palette.push_back(base);
            return palette;
        }

        palette.reserve(video.colorToneLevels);

        for (int i = 0; i < video.colorToneLevels; ++i)
        {
            const float t =
                static_cast<float>(i) /
                static_cast<float>(video.colorToneLevels - 1);

            const float lightFactor =
                1.0f - t;

            const float darkFactor =
                0.25f + 0.75f * t;

            Color tone;
            tone.r =
                static_cast<unsigned char>(
                    std::clamp(
                        static_cast<int>(
                            std::round(
                                static_cast<float>(base.r) * darkFactor +
                                255.0f * lightFactor * 0.35f
                            )
                        ),
                        0,
                        255
                    )
                );
            tone.g =
                static_cast<unsigned char>(
                    std::clamp(
                        static_cast<int>(
                            std::round(
                                static_cast<float>(base.g) * darkFactor +
                                255.0f * lightFactor * 0.35f
                            )
                        ),
                        0,
                        255
                    )
                );
            tone.b =
                static_cast<unsigned char>(
                    std::clamp(
                        static_cast<int>(
                            std::round(
                                static_cast<float>(base.b) * darkFactor +
                                255.0f * lightFactor * 0.35f
                            )
                        ),
                        0,
                        255
                    )
                );
            tone.a = 255;

            palette.push_back(tone);
        }

        return palette;
    }
}

Color VideoColorProcessor::project(
    Color color,
    const VideoChipDefinition& video
)
{
    if (!video.colorAlpha)
    {
        color.a = 255;
    }

    if (video.hasColorPalette)
    {
        return closestColor(
            color,
            video.colorPalette
        );
    }

    if (video.hasColorTone)
    {
        return closestColor(
            color,
            buildTonePalette(video)
        );
    }

    if (video.hasColorLevels)
    {
        return Color{
            quantizeChannel(color.r, video.colorLevelsRed),
            quantizeChannel(color.g, video.colorLevelsGreen),
            quantizeChannel(color.b, video.colorLevelsBlue),
            color.a
        };
    }

    return color;
}
