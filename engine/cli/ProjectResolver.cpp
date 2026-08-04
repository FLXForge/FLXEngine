#include "ProjectResolver.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <system_error>
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

    void addFilesystemError(
        ProjectResolutionResult& result,
        const std::filesystem::path& path,
        const std::error_code& error
    )
    {
        result.success = false;
        result.exitCode = CliExitCode::ProjectResolutionError;
        result.diagnostics.error(
            DiagnosticCode::ProjectErrorUnclassified,
            "Filesystem error while resolving project: " + error.message(),
            path.generic_string()
        );
    }

    bool hasFilesystemError(
        ProjectResolutionResult& result,
        const std::filesystem::path& path,
        const std::error_code& error
    )
    {
        if (!error)
        {
            return false;
        }

        addFilesystemError(result, path, error);
        return true;
    }
}

ProjectResolutionResult ProjectResolver::resolve(
    const std::filesystem::path& target
) const
{
    ProjectResolutionResult result;
    std::error_code error;

    error.clear();
    const bool exists =
        std::filesystem::exists(target, error);

    if (hasFilesystemError(result, target, error))
    {
        return result;
    }

    error.clear();
    const bool isFile =
        std::filesystem::is_regular_file(target, error);

    if (hasFilesystemError(result, target, error))
    {
        return result;
    }

    if (isFile)
    {
        if (!isFlxFile(target))
        {
            result.diagnostics.error(
                DiagnosticCode::ProjectErrorUnclassified,
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

    if (!exists)
    {
        result.diagnostics.error(
            DiagnosticCode::ProjectErrorUnclassified,
            "Project target was not found",
            target.generic_string()
        );
        return result;
    }

    error.clear();
    const bool isDirectory =
        std::filesystem::is_directory(target, error);

    if (hasFilesystemError(result, target, error))
    {
        return result;
    }

    if (!isDirectory)
    {
        result.diagnostics.error(
            DiagnosticCode::ProjectErrorUnclassified,
            "Project target is not a .flx file or directory",
            target.generic_string()
        );
        return result;
    }

    const std::filesystem::path projectManifest =
        target / "project.flx";

    error.clear();
    const bool projectManifestExists =
        std::filesystem::exists(projectManifest, error);

    if (hasFilesystemError(result, projectManifest, error))
    {
        return result;
    }

    if (!projectManifestExists)
    {
        error.clear();
    }

    error.clear();
    const bool hasProjectManifest =
        projectManifestExists &&
        std::filesystem::is_regular_file(projectManifest, error);

    if (hasFilesystemError(result, projectManifest, error))
    {
        return result;
    }

    if (hasProjectManifest)
    {
        result.success = true;
        result.exitCode = CliExitCode::Success;
        result.manifestPath = projectManifest;
        return result;
    }

    std::vector<std::filesystem::path> manifests;

    error.clear();
    std::filesystem::directory_iterator iterator(
        target,
        error
    );

    if (hasFilesystemError(result, target, error))
    {
        return result;
    }

    const std::filesystem::directory_iterator end;

    while (iterator != end)
    {
        const std::filesystem::directory_entry entry =
            *iterator;

        error.clear();
        const bool entryIsFile =
            entry.is_regular_file(error);

        if (hasFilesystemError(result, entry.path(), error))
        {
            return result;
        }

        if (entryIsFile && isFlxFile(entry.path()))
        {
            manifests.push_back(entry.path());
        }

        error.clear();
        iterator.increment(error);

        if (hasFilesystemError(result, target, error))
        {
            return result;
        }
    }

    std::sort(manifests.begin(), manifests.end());

    if (manifests.empty())
    {
        result.diagnostics.error(
            DiagnosticCode::ProjectErrorUnclassified,
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
            DiagnosticCode::ProjectErrorUnclassified,
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
