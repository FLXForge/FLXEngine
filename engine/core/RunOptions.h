#pragma once

#include <optional>

enum class WindowMode
{
    Window,
    Fullscreen
};

struct RunOptions
{
    std::optional<int> maxFrames;
    std::optional<int> scaleOverride;
    WindowMode windowMode = WindowMode::Window;
    bool debugCollisions = false;
    bool debugLogs = false;
    bool debugConsole = false;
};
