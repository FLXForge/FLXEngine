#include "VersionCommand.h"

#include "../CliExitCode.h"
#include "FlxVersion.h"

#include <iostream>

int VersionCommand::execute() const
{
    std::cout << FlxVersion::Text << "\n";
    return static_cast<int>(CliExitCode::Success);
}
