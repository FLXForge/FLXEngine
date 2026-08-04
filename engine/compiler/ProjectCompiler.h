#pragma once

#include "CompiledProject.h"

#include <filesystem>
#include <unordered_set>
#include <string>

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

    static ObjectDefinition compileDefinition(
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

    static void compileInputMapping(
        FlxContext& context,
        Diagnostics& diagnostics
    );
};
