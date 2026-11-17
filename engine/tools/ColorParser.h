#pragma once

#include <raylib.h>

#include <string>
#include <optional>

class ColorParser
{
public:
    // Native names and #RGB/#RRGGBB only; no logging or fallback.
    static std::optional<Color> tryParse(const std::string& value);
    static Color parse(
        const std::string& value,
        Color fallback = WHITE
    );
};
