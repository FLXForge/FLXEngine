#pragma once

#include "../diagnostics/Diagnostics.h"
#include "ResourceRegistry.h"
#include "../project/FlxContext.h"
#include "../runtime/ObjectDefinition.h"

#include <string>

struct CompiledProject
{
    FlxContext context;
    ResourceRegistry resources;
    ResourceId rootId;
    ObjectDefinition rootDefinition;
    std::string rootPath;
};

struct CompilationResult
{
    bool success = false;
    CompiledProject project;
    Diagnostics diagnostics;
};
