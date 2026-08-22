#pragma once

#include "CompiledProject.h"

#include <filesystem>
#include <unordered_set>
#include <string>

struct ProjectManifest;

class ProjectCompiler
{
public:
    CompilationResult compile(const std::string& projectPath) const;

private:
    static void projectDefinitionColors(
        ObjectDefinition& definition,
        const VideoChipDefinition& video
    );

    static ResourceId makeResourceId(
        const ObjectDefinition& definition,
        const std::string& fallbackPath,
        const std::filesystem::path& projectRoot
    );

    static void compileDefinition(
        const ObjectDefinition& definition,
        const VideoChipDefinition& video,
        ResourceRegistry& registry,
        Diagnostics& diagnostics,
        std::unordered_set<ResourceId>& compiling,
        const std::filesystem::path& projectRoot,
        const std::filesystem::path& worldRoot
    );

    static void resolveScripts(
        ObjectDefinition& definition,
        ResourceRegistry& registry,
        Diagnostics& diagnostics,
        const std::string& sourcePath,
        const std::filesystem::path& projectRoot,
        const std::filesystem::path& worldRoot
    );

    static std::string relativeSourceName(
        const std::string& path,
        const std::filesystem::path& projectRoot
    );

    static std::string readTextFile(const std::string& path);

    static std::filesystem::path resolveFrom(
        const std::filesystem::path& basePath,
        const std::string& value
    );

    static FlxContext makeCompiledContext(
        const ProjectManifest& manifest,
        const std::filesystem::path& manifestDirectory,
        Diagnostics& diagnostics
    );

    static void compileInputMapping(
        FlxContext& context,
        const std::string& inputMappingPath,
        const std::filesystem::path& manifestDirectory,
        Diagnostics& diagnostics
    );

};
