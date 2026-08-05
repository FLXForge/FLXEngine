#include "ProjectManifestLoader.h"

#include <fstream>
#include <string>
#include <unordered_set>

namespace
{
    bool assignField(
        ProjectManifest& manifest,
        const std::string& key,
        const std::string& value
    )
    {
        if (key == "name")
        {
            manifest.metadata.name = value;
            return true;
        }

        if (key == "version")
        {
            manifest.metadata.version = value;
            return true;
        }

        if (key == "notes")
        {
            manifest.metadata.notes = value;
            return true;
        }

        if (key == "title")
        {
            manifest.title = value;
            return true;
        }

        if (key == "engine")
        {
            manifest.engineRequirement = value;
            return true;
        }

        if (key == "path")
        {
            manifest.path = value.empty() ? "." : value;
            return true;
        }

        if (key == "root")
        {
            manifest.root = value;
            return true;
        }

        if (key == "machine")
        {
            manifest.machine = value;
            return true;
        }

        if (key == "input.mapping")
        {
            manifest.inputMapping = value;
            return true;
        }

        return false;
    }
}

ProjectManifestResult ProjectManifestLoader::load(const std::string& path)
{
    ProjectManifestResult result;

    std::ifstream file(path);

    if (!file.is_open())
    {
        result.diagnostics.error(
            DiagnosticCode::ProjectManifestCouldNotBeOpened,
            "Project manifest could not be opened",
            path
        );

        return result;
    }

    std::unordered_set<std::string> seenKeys;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        const std::string trimmed =
            trim(line);

        if (trimmed.empty() || trimmed.front() == '#')
        {
            continue;
        }

        const std::size_t separator =
            trimmed.find('=');

        if (separator == std::string::npos)
        {
            result.diagnostics.error(
                DiagnosticCode::InvalidProjectManifestSyntax,
                "Invalid project manifest line. Expected key=value.",
                path,
                "",
                SourceRange{
                    SourcePosition{ lineNumber, 1 },
                    SourcePosition{ lineNumber, static_cast<int>(trimmed.size()) }
                }
            );

            continue;
        }

        const std::string key =
            trim(trimmed.substr(0, separator));

        const std::string value =
            trim(trimmed.substr(separator + 1));

        if (seenKeys.contains(key))
        {
            result.diagnostics.error(
                DiagnosticCode::DuplicateProjectManifestField,
                "Duplicate project manifest field: " + key,
                path,
                key
            );

            continue;
        }

        seenKeys.insert(key);

        if (!assignField(result.manifest, key, value))
        {
            result.diagnostics.error(
                DiagnosticCode::UnknownProjectManifestField,
                "Unknown project manifest field: " + key,
                path,
                key
            );
        }
    }

    if (result.manifest.root.empty())
    {
        result.diagnostics.error(
            DiagnosticCode::MissingProjectRoot,
            "Project manifest must define a non-empty root field",
            path,
            "root"
        );
    }

    if (result.manifest.title.empty())
    {
        result.manifest.title =
            result.manifest.metadata.name;
    }

    result.success =
        !result.diagnostics.hasErrors();

    return result;
}

std::string ProjectManifestLoader::trim(const std::string& value)
{
    const std::size_t start =
        value.find_first_not_of(" \t\r\n");

    if (start == std::string::npos)
    {
        return "";
    }

    const std::size_t end =
        value.find_last_not_of(" \t\r\n");

    return value.substr(start, end - start + 1);
}
