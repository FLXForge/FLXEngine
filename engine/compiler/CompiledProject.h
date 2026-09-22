#pragma once

#include "../diagnostics/Diagnostics.h"
#include "ResourceRegistry.h"
#include "../project/FlxContext.h"

#include <string>

struct CompiledProject
{
    FlxContext context;
    ResourceRegistry resources;
    ResourceId rootId;
};

struct CompilationResult
{
    bool success = false;
    CompiledProject project;
    Diagnostics diagnostics;
};
