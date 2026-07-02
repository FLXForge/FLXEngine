#pragma once

#include "MachineDefinition.h"

#include <raylib.h>

class VideoColorProcessor
{
public:
    static Color project(
        Color color,
        const VideoChipDefinition& video
    );
};
