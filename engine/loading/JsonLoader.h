#pragma once

#include "../diagnostics/Diagnostics.h"
#include "../runtime/ObjectDefinition.h"

#include <string>

class JsonLoader
{
public:
    static std::string resolveProjectPath(
        const std::string& projectPath,
        const std::string& path,
        const std::string& extension
    );

    static std::string resolveReferencedPath(
        const std::string& sourceFile,
        const std::string& path,
        const std::string& extension
    );

    static ObjectDefinition loadObjectDefinition(
        const std::string& path
    );

    static ObjectDefinition loadObjectDefinition(
        const std::string& path,
        const std::string& projectRoot,
        Diagnostics& diagnostics
    );
};
