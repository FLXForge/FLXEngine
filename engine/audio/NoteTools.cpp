#include "NoteTools.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <string>
#include <unordered_map>

bool NoteTools::noteToFrequency(
    const std::string& note,
    float& frequency
)
{
    if (note.size() < 2)
    {
        return false;
    }

    std::string normalized;
    normalized.reserve(note.size());

    for (char character : note)
    {
        if (!std::isspace(static_cast<unsigned char>(character)))
        {
            normalized.push_back(
                static_cast<char>(
                    std::toupper(static_cast<unsigned char>(character))
                )
            );
        }
    }

    if (normalized.size() < 2)
    {
        return false;
    }

    size_t octaveIndex = 1;
    std::string pitch =
        normalized.substr(0, 1);

    if (
        normalized.size() >= 3 &&
        (normalized[1] == '#' || normalized[1] == 'B')
    )
    {
        pitch =
            normalized.substr(0, 2);
        octaveIndex = 2;
    }

    if (octaveIndex >= normalized.size())
    {
        return false;
    }

    int octave = 0;

    try
    {
        octave =
            std::stoi(normalized.substr(octaveIndex));
    }
    catch (...)
    {
        return false;
    }

    if (octave < 0 || octave > 8)
    {
        return false;
    }

    static const std::unordered_map<std::string, int> semitones{
        { "C", 0 },
        { "C#", 1 },
        { "DB", 1 },
        { "D", 2 },
        { "D#", 3 },
        { "EB", 3 },
        { "E", 4 },
        { "F", 5 },
        { "F#", 6 },
        { "GB", 6 },
        { "G", 7 },
        { "G#", 8 },
        { "AB", 8 },
        { "A", 9 },
        { "A#", 10 },
        { "BB", 10 },
        { "B", 11 }
    };

    const auto it =
        semitones.find(pitch);

    if (it == semitones.end())
    {
        return false;
    }

    const int midi =
        (octave + 1) * 12 + it->second;

    frequency =
        440.0f * std::pow(2.0f, static_cast<float>(midi - 69) / 12.0f);

    return true;
}
