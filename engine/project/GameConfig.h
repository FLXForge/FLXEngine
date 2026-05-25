#pragma once

#include <string>
#include <vector>

struct GameConfig
{
    std::string name;
    std::string description;

    std::string screenTitle;
    int screenWidth = 320;
    int screenHeight = 180;
    int scale = 3;

    std::vector<std::string> children;
};