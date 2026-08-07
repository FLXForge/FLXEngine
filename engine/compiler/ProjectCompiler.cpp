#include "ProjectCompiler.h"
#include "CompiledProjectValidator.h"
#include "../loading/JsonLoader.h"
#include "../machine/MachineLoader.h"
#include "../machine/VideoColorProcessor.h"
#include "../project/ProjectManifestLoader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

CompilationResult ProjectCompiler::compile(
    const std::string& projectPath
) const
{
    CompilationResult result;

    try
    {
        const std::filesystem::path manifestPath =
            std::filesystem::path(projectPath);

        const std::filesystem::path manifestDirectory =
            manifestPath.parent_path().empty()
            ? std::filesystem::path(".")
            : manifestPath.parent_path();

        ProjectManifestResult manifestResult =
            ProjectManifestLoader::load(projectPath);

        result.diagnostics.append(manifestResult.diagnostics);

        if (!manifestResult.success)
        {
            return result;
        }

        result.project.context =
            makeCompiledContext(
                manifestResult.manifest,
                manifestDirectory,
                result.diagnostics
            );

        compileInputMapping(
            result.project.context,
            manifestResult.manifest.inputMapping,
            manifestDirectory,
            result.diagnostics
        );

        if (result.diagnostics.hasErrors())
        {
            return result;
        }

        const std::filesystem::path worldRoot =
            resolveFrom(
                manifestDirectory,
                manifestResult.manifest.path
            );

        const std::string rootPath =
            JsonLoader::resolveProjectPath(
                worldRoot.generic_string(),
                manifestResult.manifest.root,
                ".json"
            );

        if (!std::filesystem::exists(rootPath))
        {
            result.diagnostics.error(
                DiagnosticCode::ResourceErrorUnclassified,
                "Root JSON does not exist",
                rootPath
            );

            return result;
        }

        ObjectDefinition rootDefinition =
            JsonLoader::loadObjectDefinition(
                rootPath,
                worldRoot.generic_string(),
                result.diagnostics
            );

        result.project.rootId =
            makeResourceId(
                rootDefinition,
                rootPath,
                manifestDirectory
            );

        std::unordered_set<ResourceId> compiling;

        compileDefinition(
            rootDefinition,
            result.project.context.machine.video,
            result.project.resources,
            result.diagnostics,
            compiling,
            manifestDirectory,
            worldRoot
        );

        CompiledProjectValidator::validate(
            result.project,
            result.diagnostics,
            projectPath
        );
    }
    catch (const std::exception& exception)
    {
        result.diagnostics.error(
            DiagnosticCode::CompErrorUnclassified,
            exception.what(),
            projectPath
        );

        return result;
    }

    result.success =
        !result.diagnostics.hasErrors();

    if (result.success)
    {
        result.diagnostics.info(
            DiagnosticCode::CompInformationUnclassified,
            "Project compiled",
            projectPath
        );
    }

    return result;
}

std::filesystem::path ProjectCompiler::resolveFrom(
    const std::filesystem::path& basePath,
    const std::string& value
)
{
    if (value.empty() || value == ".")
    {
        return basePath.lexically_normal();
    }

    const std::filesystem::path path(value);

    if (path.is_absolute())
    {
        return path.lexically_normal();
    }

    return (basePath / path).lexically_normal();
}

FlxContext ProjectCompiler::makeCompiledContext(
    const ProjectManifest& manifest,
    const std::filesystem::path& manifestDirectory,
    Diagnostics& diagnostics
)
{
    FlxContext context;
    context.name = manifest.metadata.name;
    context.version = manifest.metadata.version;
    context.notes = manifest.metadata.notes;
    context.title = manifest.title;
    context.engineRequirement = manifest.engineRequirement;

    if (manifest.machine.empty())
    {
        context.machine =
            MachineLoader::defaultMachine();
    }
    else
    {
        const std::filesystem::path machinePath =
            resolveFrom(
                manifestDirectory,
                manifest.machine
            );

        if (!std::filesystem::exists(machinePath))
        {
            diagnostics.error(
                DiagnosticCode::MachineErrorUnclassified,
                "Machine file does not exist",
                machinePath.generic_string(),
                "machine"
            );

            context.machine =
                MachineLoader::defaultMachine();
        }
        else
        {
            context.machine =
                MachineLoader::load(machinePath.generic_string());
        }
    }

    return context;
}

void ProjectCompiler::projectDefinitionColors(
    ObjectDefinition& definition,
    const VideoChipDefinition& video
)
{
    definition.color =
        VideoColorProcessor::project(
            definition.color,
            video
        );

    for (auto& child : definition.children)
    {
        projectDefinitionColors(
            child.second,
            video
        );
    }
}

ResourceId ProjectCompiler::makeResourceId(
    const ObjectDefinition& definition,
    const std::string& fallbackPath,
    const std::filesystem::path& projectRoot
)
{
    const std::string path =
        definition.sourcePath.empty()
        ? fallbackPath
        : definition.sourcePath;

    ResourceId id =
        std::filesystem::path(path).is_absolute()
        ? relativeSourceName(path, projectRoot)
        : std::filesystem::path(path).lexically_normal().generic_string();

    id +=
        "#" +
        definition.id;

    if (definition.spawnMode != "auto")
    {
        id +=
            "@spawn=" +
            definition.spawnMode;
    }

    return id;
}

ObjectDefinition ProjectCompiler::compileDefinition(
    const ObjectDefinition& definition,
    const VideoChipDefinition& video,
    ResourceRegistry& registry,
    Diagnostics& diagnostics,
    std::unordered_set<ResourceId>& compiling,
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& worldRoot
)
{
    const ResourceId id =
        makeResourceId(
            definition,
            definition.sourcePath,
            projectRoot
        );

    if (registry.hasObject(id))
    {
        const ObjectDefinition* existing =
            registry.findObject(id);

        return existing == nullptr
            ? definition
            : *existing;
    }

    if (compiling.find(id) != compiling.end())
    {
        ObjectDefinition placeholder =
            definition;
        placeholder.children.clear();
        placeholder.childResources.clear();

        return placeholder;
    }

    compiling.insert(id);

    ObjectDefinition compiled =
        definition;

    compiled.color =
        VideoColorProcessor::project(
            compiled.color,
            video
        );

    resolveScripts(
        compiled,
        registry,
        diagnostics,
        definition.sourcePath,
        projectRoot,
        worldRoot
    );

    compiled.sourcePath =
        relativeSourceName(
            definition.sourcePath,
            projectRoot
        );

    compiled.childResources.clear();

    for (const auto& child : definition.children)
    {
        ObjectDefinition compiledChild =
            compileDefinition(
                child.second,
                video,
                registry,
                diagnostics,
                compiling,
                projectRoot,
                worldRoot
            );

        const ResourceId childResourceId =
            makeResourceId(
                child.second,
                child.second.sourcePath,
                projectRoot
            );

        compiled.childResources[child.first] =
            childResourceId;

    }

    compiled.children.clear();

    compiling.erase(id);

    if (!registry.addObject(id, compiled))
    {
        diagnostics.error(
            DiagnosticCode::ResourceIdCollision,
            "Resource id collision detected",
            compiled.sourcePath,
            id
        );
    }

    return compiled;
}

void ProjectCompiler::resolveScripts(
    ObjectDefinition& definition,
    ResourceRegistry& registry,
    Diagnostics& diagnostics,
    const std::string& sourcePath,
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& worldRoot
)
{
    definition.resolvedScriptPaths.clear();

    for (std::size_t i = 0; i < definition.scripts.size(); ++i)
    {
        const std::string& script =
            definition.scripts[i];
        const std::string scriptSourcePath =
            i < definition.scriptSourcePaths.size()
            ? definition.scriptSourcePaths[i]
            : sourcePath;

        const std::string scriptPath =
            !script.empty() && script.front() == '/'
            ? JsonLoader::resolveProjectPath(
                worldRoot.generic_string(),
                script.substr(1),
                ".js"
            )
            : JsonLoader::resolveReferencedPath(
                scriptSourcePath,
                script,
                ".js"
            );

        if (!std::filesystem::exists(scriptPath))
        {
            diagnostics.error(
                DiagnosticCode::ReferencedScriptNotFound,
                "Script does not exist\nReference: " + script +
                "\nResolved path: " + scriptPath,
                scriptSourcePath,
                "behavior.scripts"
            );

            continue;
        }

        ScriptResource resource;
        resource.id =
            relativeSourceName(
                scriptPath,
                projectRoot
            );
        resource.sourceName =
            resource.id;
        resource.code =
            readTextFile(scriptPath);

        const ScriptResource* existing =
            registry.findScript(resource.id);

        if (existing != nullptr)
        {
            if (
                existing->sourceName != resource.sourceName ||
                existing->code != resource.code
            )
            {
                diagnostics.error(
                    DiagnosticCode::ResourceIdCollision,
                    "Script resource id collision detected",
                    scriptSourcePath,
                    resource.id
                );
            }
        }
        else if (!registry.addScript(resource.id, resource))
        {
            diagnostics.error(
                DiagnosticCode::ResourceIdCollision,
                "Script resource id collision detected",
                scriptSourcePath,
                resource.id
            );
        }

        definition.resolvedScriptPaths.push_back(resource.id);
    }
}

std::string ProjectCompiler::relativeSourceName(
    const std::string& path,
    const std::filesystem::path& projectRoot
)
{
    std::filesystem::path sourcePath(path);

    if (sourcePath.empty())
    {
        return "";
    }

    sourcePath =
        std::filesystem::absolute(sourcePath).lexically_normal();

    std::filesystem::path root =
        std::filesystem::absolute(projectRoot).lexically_normal();

    std::error_code error;
    const std::filesystem::path relative =
        std::filesystem::relative(
            sourcePath,
            root,
            error
        );

    if (!error && !relative.empty())
    {
        return relative.generic_string();
    }

    return sourcePath.filename().generic_string();
}

std::string ProjectCompiler::readTextFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

void ProjectCompiler::compileInputMapping(
    FlxContext& context,
    const std::string& inputMappingPath,
    const std::filesystem::path& manifestDirectory,
    Diagnostics& diagnostics
)
{
    if (inputMappingPath.empty())
    {
        return;
    }

    const std::filesystem::path resolvedPath =
        resolveFrom(
            manifestDirectory,
            inputMappingPath
        );

    if (!std::filesystem::exists(resolvedPath))
    {
        diagnostics.error(
            DiagnosticCode::ResourceErrorUnclassified,
            "Input mapping does not exist",
            resolvedPath.generic_string(),
            "input.mapping"
        );

        return;
    }

    context.inputMappingSourceName =
        relativeSourceName(
            resolvedPath.generic_string(),
            manifestDirectory
        );

    context.inputMappingContent =
        readTextFile(resolvedPath.generic_string());
}

