#pragma once

#include "CompiledProject.h"

#include <string>

class CompiledProjectValidator
{
public:
    static bool validate(
        const CompiledProject& project,
        Diagnostics& diagnostics,
        DiagnosticCode code,
        const std::string& source
    );
};
