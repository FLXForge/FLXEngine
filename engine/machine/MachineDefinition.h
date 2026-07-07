#pragma once

#include <raylib.h>

#include <string>
#include <vector>

struct VideoChipDefinition
{
    int screenWidth = 640;
    int screenHeight = 480;
    std::string clearColor = "black";

    std::vector<Color> colorPalette;
    bool hasColorPalette = false;

    int colorLevelsRed = 0;
    int colorLevelsGreen = 0;
    int colorLevelsBlue = 0;
    bool hasColorLevels = false;

    std::string colorToneBase = "";
    int colorToneLevels = 0;
    bool hasColorTone = false;

    bool colorAlpha = true;

    bool planesEnabled = false;
    bool objectsSprites = false;

    int outputScale = 1;
    bool smoothing = false;
};

struct AudioChipDefinition
{
    int voicesMusic = 8;
    int voicesSound = 16;
    std::string voicesMode = "shared";
    std::string voicesOverflow = "replace_oldest";

    std::string synthesisModel = "open";
    std::string synthesisTexture = "rich";
    std::string synthesisMovement = "expressive";
    std::string synthesisNoise = "rich";

    std::string fidelityResolution = "high";
    std::string fidelityDynamics = "expressive";
    std::string fidelitySpace = "stereo";

    bool resourcesGenerated = true;
    bool resourcesSamples = true;
    bool resourcesStreams = true;

    std::string fileAudioMode = "all";
};

struct InputChipDefinition
{
    int players = 16;
    std::string direction = "analog";
    int playerButtons = 16;
    int systemButtons = 16;
    bool pointer = true;
    bool text = true;
};

struct MachineDefinition
{
    VideoChipDefinition video;
    AudioChipDefinition audio;
    InputChipDefinition input;
};
