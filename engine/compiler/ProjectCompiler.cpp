#include "ProjectCompiler.h"
#include "../loading/JsonLoader.h"
#include "../machine/VideoColorProcessor.h"
#include "../project/FlxContextBuilder.h"

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

    if (!std::filesystem::exists(projectPath))
    {
        result.diagnostics.error(
            DiagnosticCode::CompErrorUnclassified,
            "Project file does not exist",
            projectPath
        );

        return result;
    }

    try
    {
        result.project.context =
            FlxContextBuilder::build(projectPath);

        compileInputMapping(
            result.project.context,
            result.diagnostics
        );

        if (result.project.context.root.empty())
        {
            result.diagnostics.error(
                DiagnosticCode::CompErrorUnclassified,
                "Project root is not defined",
                projectPath,
                "root"
            );

            return result;
        }

        result.project.rootPath =
            JsonLoader::resolveProjectPath(
                result.project.context.projectPath,
                result.project.context.root,
                ".json"
            );

        if (!std::filesystem::exists(result.project.rootPath))
        {
            result.diagnostics.error(
                DiagnosticCode::ResourceErrorUnclassified,
                "Root JSON does not exist",
                result.project.rootPath
            );

            return result;
        }

        ObjectDefinition rootDefinition =
            JsonLoader::loadObjectDefinition(result.project.rootPath);

        result.project.rootId =
            makeResourceId(
                rootDefinition,
                result.project.rootPath,
                result.project.context.rootDirectory
            );

        std::unordered_set<ResourceId> compiling;

        result.project.rootDefinition =
            compileDefinition(
                rootDefinition,
                result.project.context.machine.video,
                result.project.resources,
                result.diagnostics,
                compiling,
                result.project.context.rootDirectory
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

    if (!std::filesystem::path(path).is_absolute())
    {
        return
            std::filesystem::path(path).lexically_normal().generic_string() +
            "#" +
            definition.id;
    }

    return
        relativeSourceName(path, projectRoot) +
        "#" +
        definition.id;
}

ObjectDefinition ProjectCompiler::compileDefinition(
    const ObjectDefinition& definition,
    const VideoChipDefinition& video,
    ResourceRegistry& registry,
    Diagnostics& diagnostics,
    std::unordered_set<ResourceId>& compiling,
    const std::filesystem::path& projectRoot
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
        diagnostics.error(
            DiagnosticCode::ResourceErrorUnclassified,
            "Resource cycle detected while compiling object graph",
            definition.sourcePath,
            id
        );

        return definition;
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
        projectRoot
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
                projectRoot
            );

        const ResourceId childResourceId =
            makeResourceId(
                compiledChild,
                child.second.sourcePath,
                projectRoot
            );

        compiled.childResources[child.first] =
            childResourceId;

        compiled.children[child.first] =
            compiledChild;
    }

    compiling.erase(id);

    registry.addObject(
        id,
        compiled
    );

    return compiled;
}

void ProjectCompiler::resolveScripts(
    ObjectDefinition& definition,
    ResourceRegistry& registry,
    Diagnostics& diagnostics,
    const std::string& sourcePath,
    const std::filesystem::path& projectRoot
)
{
    definition.resolvedScriptPaths.clear();

    for (const std::string& script : definition.scripts)
    {
        const std::string scriptPath =
            JsonLoader::resolveReferencedPath(
                sourcePath,
                script,
                ".js"
            );

        if (!std::filesystem::exists(scriptPath))
        {
            diagnostics.error(
                DiagnosticCode::ResourceErrorUnclassified,
                "Script does not exist",
                scriptPath,
                definition.id
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

        registry.addScript(
            resource.id,
            resource
        );

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
    Diagnostics& diagnostics
)
{
    if (context.inputMappingPath.empty())
    {
        return;
    }

    if (!std::filesystem::exists(context.inputMappingPath))
    {
        diagnostics.error(
            DiagnosticCode::ResourceErrorUnclassified,
            "Input mapping does not exist",
            context.inputMappingPath
        );

        return;
    }

    context.inputMappingSourceName =
        relativeSourceName(
            context.inputMappingPath,
            context.rootDirectory
        );

    context.inputMappingContent =
        readTextFile(context.inputMappingPath);
}

