#pragma once

#include <raylib.h>

#include <string>

class ColorParser
{
public:
    static Color parse(
        const std::string& value,
        Color fallback = WHITE
    );
};
