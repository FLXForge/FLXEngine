#include "ProjectCompiler.h"
#include "CompiledProjectValidator.h"
#include "../loading/JsonLoader.h"
#include "../input/InputMappingLoader.h"
#include "../machine/MachineLoader.h"
#include "../machine/VideoColorProcessor.h"
#include "../project/ProjectManifestLoader.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace
{
    void fillCompiledDefinitionShell(
        ObjectDefinition& compiled,
        const ObjectDefinition& definition
    )
    {
        compiled.id = definition.id;
        compiled.sourcePath = definition.sourcePath;
        compiled.spawnMode = definition.spawnMode;
        compiled.component = definition.component;

        compiled.offset = definition.offset;
        compiled.hasOffset = definition.hasOffset;

        compiled.attachFollowX = definition.attachFollowX;
        compiled.attachFollowY = definition.attachFollowY;
        compiled.attachFollowAngle = definition.attachFollowAngle;
        compiled.attachOnCreate = definition.attachOnCreate;

        compiled.visible = definition.visible;
        compiled.hasVisual = definition.hasVisual;
        compiled.layer = definition.layer;

        compiled.origin = definition.origin;
        compiled.hasOrigin = definition.hasOrigin;
        compiled.size = definition.size;
        compiled.color = definition.color;
        compiled.shapeMode = definition.shapeMode;
        compiled.shapeType = definition.shapeType;
        compiled.textContent = definition.textContent;
        compiled.radius = definition.radius;
        compiled.points = definition.points;

        compiled.mechanics = definition.mechanics;
        compiled.inherit = definition.inherit;

        compiled.boundsMode = definition.boundsMode;
        compiled.boundsOverflow = definition.boundsOverflow;

        compiled.group = definition.group;
        compiled.controlPlayer = definition.controlPlayer;
        compiled.local = definition.local;
        compiled.collisions = definition.collisions;

        compiled.scripts = definition.scripts;
        compiled.scriptSourcePaths = definition.scriptSourcePaths;
        compiled.resolvedScriptPaths = definition.resolvedScriptPaths;
        compiled.music = definition.music;
        compiled.sounds = definition.sounds;
        compiled.childSourcePaths = definition.childSourcePaths;

        compiled.initialState = definition.initialState;
        compiled.stateTransitions = definition.stateTransitions;

        compiled.creationMode = definition.creationMode;
        compiled.gridRules = definition.gridRules;
        compiled.gridPatternIsRows = definition.gridPatternIsRows;
        compiled.gridPattern = definition.gridPattern;
        compiled.gridRowPattern = definition.gridRowPattern;
    }
}

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

        if (rootDefinition.component)
        {
            result.diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Root object cannot declare component=true",
                rootPath,
                "component"
            );

            return result;
        }

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

        result.success =
            !result.diagnostics.hasErrors();
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
                MachineLoader::load(
                    machinePath.generic_string(),
                    diagnostics
                );
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
        if (!child.second)
        {
            continue;
        }

        projectDefinitionColors(
            *child.second,
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

void ProjectCompiler::compileDefinition(
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
        return;
    }

    if (compiling.find(id) != compiling.end())
    {
        return;
    }

    compiling.insert(id);
    auto compiled =
        std::make_unique<ObjectDefinition>();

    fillCompiledDefinitionShell(
        *compiled,
        definition
    );

    compiled->color =
        VideoColorProcessor::project(
            compiled->color,
            video
        );

    resolveScripts(
        *compiled,
        registry,
        diagnostics,
        definition.sourcePath,
        projectRoot,
        worldRoot
    );

    compiled->sourcePath =
        relativeSourceName(
            definition.sourcePath,
            projectRoot
        );

    compiled->childResources.clear();

    for (const auto& child : definition.children)
    {
        if (!child.second)
        {
            continue;
        }

        compileDefinition(
            *child.second,
            video,
            registry,
            diagnostics,
            compiling,
            projectRoot,
            worldRoot
        );

        const ResourceId childResourceId =
            makeResourceId(
                *child.second,
                child.second->sourcePath,
                projectRoot
            );

        compiled->childResources[child.first] =
            childResourceId;
    }

    compiled->children.clear();

    compiling.erase(id);

    const bool objectAdded =
        registry.addObjectOwned(id, std::move(compiled));

    if (!objectAdded)
    {
        diagnostics.error(
            DiagnosticCode::ResourceIdCollision,
            "Resource id collision detected",
            definition.sourcePath,
            id
        );
    }
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
        const InputMappingLoadResult mappingResult =
            InputMappingLoader::loadDefault(context.machine.input);

        diagnostics.append(mappingResult.diagnostics);

        if (!mappingResult.success)
        {
            return;
        }

        context.inputMappingSourceName =
            mappingResult.sourceName;

        context.inputMappingContent =
            mappingResult.content;

        context.inputMapping =
            mappingResult.mapping;

        return;
    }

    const std::filesystem::path resolvedPath =
        resolveFrom(
            manifestDirectory,
            inputMappingPath
        );

    if (!std::filesystem::exists(resolvedPath))
    {
        const InputMappingLoadResult mappingResult =
            InputMappingLoader::loadFile(
                resolvedPath.generic_string(),
                context.machine.input
            );

        diagnostics.append(mappingResult.diagnostics);

        return;
    }

    const std::string content =
        readTextFile(resolvedPath.generic_string());

    const InputMappingLoadResult mappingResult =
        InputMappingLoader::loadContent(
            relativeSourceName(
                resolvedPath.generic_string(),
                manifestDirectory
            ),
            content,
            context.machine.input
        );

    diagnostics.append(mappingResult.diagnostics);

    if (!mappingResult.success)
    {
        return;
    }

    context.inputMappingSourceName =
        relativeSourceName(
            resolvedPath.generic_string(),
            manifestDirectory
        );

    context.inputMappingContent =
        content;

    context.inputMapping =
        mappingResult.mapping;
}

