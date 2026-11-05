#pragma once

#include <optional>
#include <filesystem>

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
    std::optional<std::filesystem::path> savePath;
};
