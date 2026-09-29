#pragma once

#include <string>

class NoteTools
{
public:
    static bool noteToFrequency(
        const std::string& note,
        float& frequency
    );
};
