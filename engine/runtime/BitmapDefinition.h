#pragma once

#include <raylib.h>
#include <vector>

// Immutable after compilation. Colors have already been projected by Machine.
struct BitmapDefinition
{
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;

    bool valid() const
    {
        return width >= 1 && width <= 255 && height >= 1 && height <= 255 &&
            pixels.size() == static_cast<size_t>(width) * static_cast<size_t>(height);
    }
};
