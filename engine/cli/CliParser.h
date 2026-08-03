#pragma once

#include "CliArguments.h"
#include "CliExitCode.h"
#include "../compiler/Diagnostics.h"

struct CliParseResult
{
    bool success = false;
    CliArguments arguments;
    Diagnostics diagnostics;
    CliExitCode exitCode = CliExitCode::InvalidArguments;
};

class CliParser
{
public:
    CliParseResult parse(int argc, char* argv[]) const;
};
