#pragma once
#include <compiler/CompiledProject.h>
#include <input/InputMappingLoader.h>
#include <stdexcept>

inline CompiledProject sessionTestProject(const std::string& code = "")
{
    CompiledProject project;
    project.rootId = "root";
    project.context.name = "SessionTests";
    project.context.machine.video.screenWidth = 64;
    project.context.machine.video.screenHeight = 64;
    project.context.machine.video.outputScale = 1;
    project.context.inputMapping = InputMappingLoader::loadDefault(project.context.machine.input).mapping;
    ObjectDefinition root;
    root.id = "root";
    root.delimit = true;
    root.hasOrigin = root.hasSize = true;
    root.origin = { 32, 32 };
    root.size = { 64, 64 };
    if (!code.empty())
    {
        root.resolvedScriptPaths.push_back("entry");
        if (!project.resources.addScript("entry", { "entry", "entry.js", code }))
            throw std::runtime_error("script registration");
    }
    if (!project.resources.addObject("root", root)) throw std::runtime_error("root registration");
    return project;
}
