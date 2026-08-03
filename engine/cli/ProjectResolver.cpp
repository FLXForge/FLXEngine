#include "ProjectResolver.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
    bool equalsIgnoreCase(
        const std::string& left,
        const std::string& right
    )
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < left.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(left[i])) !=
                std::tolower(static_cast<unsigned char>(right[i])))
            {
                return false;
            }
        }

        return true;
    }

    bool isFlxFile(const std::filesystem::path& path)
    {
        return equalsIgnoreCase(path.extension().string(), ".flx");
    }
}

ProjectResolutionResult ProjectResolver::resolve(
    const std::filesystem::path& target
) const
{
    ProjectResolutionResult result;

    if (std::filesystem::is_regular_file(target))
    {
        if (!isFlxFile(target))
        {
            result.diagnostics.error(
                "Project target is a file, but it is not a .flx manifest",
                target.generic_string()
            );
            return result;
        }

        result.success = true;
        result.exitCode = CliExitCode::Success;
        result.manifestPath = target;
        return result;
    }

    if (!std::filesystem::exists(target))
    {
        result.diagnostics.error(
            "Project target was not found",
            target.generic_string()
        );
        return result;
    }

    if (!std::filesystem::is_directory(target))
    {
        result.diagnostics.error(
            "Project target is not a .flx file or directory",
            target.generic_string()
        );
        return result;
    }

    const std::filesystem::path projectManifest =
        target / "project.flx";

    if (std::filesystem::is_regular_file(projectManifest))
    {
        result.success = true;
        result.exitCode = CliExitCode::Success;
        result.manifestPath = projectManifest;
        return result;
    }

    std::vector<std::filesystem::path> manifests;

    for (const std::filesystem::directory_entry& entry :
        std::filesystem::directory_iterator(target))
    {
        if (entry.is_regular_file() && isFlxFile(entry.path()))
        {
            manifests.push_back(entry.path());
        }
    }

    std::sort(manifests.begin(), manifests.end());

    if (manifests.empty())
    {
        result.diagnostics.error(
            "No .flx project manifest was found in directory",
            target.generic_string()
        );
        return result;
    }

    if (manifests.size() > 1)
    {
        std::string message =
            "Project directory is ambiguous. Candidates:";

        for (const std::filesystem::path& manifest : manifests)
        {
            message += "\n- " + manifest.filename().generic_string();
        }

        result.diagnostics.error(
            message,
            target.generic_string()
        );
        return result;
    }

    result.success = true;
    result.exitCode = CliExitCode::Success;
    result.manifestPath = manifests.front();
    return result;
}
