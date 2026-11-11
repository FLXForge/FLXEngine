#pragma once

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace flx::portability
{
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            const auto base = std::filesystem::temp_directory_path();
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                directory = base / ("flx-portability-" + std::to_string(stamp) + "-" + std::to_string(attempt));
                if (std::filesystem::create_directory(directory)) return;
            }
            throw std::runtime_error("Could not create a unique portability test directory");
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }

        TemporaryDirectory(const TemporaryDirectory&) = delete;
        TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
        const std::filesystem::path& path() const { return directory; }

    private:
        std::filesystem::path directory;
    };

    class WorkingDirectory
    {
    public:
        explicit WorkingDirectory(const std::filesystem::path& directory)
            : previous(std::filesystem::current_path())
        {
            std::filesystem::current_path(directory);
        }
        ~WorkingDirectory() { std::filesystem::current_path(previous); }
        WorkingDirectory(const WorkingDirectory&) = delete;
        WorkingDirectory& operator=(const WorkingDirectory&) = delete;

    private:
        std::filesystem::path previous;
    };
}
