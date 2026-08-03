#include "VersionCommand.h"

#include "FlxVersion.h"

#include <iostream>

int VersionCommand::execute() const
{
    std::cout << FlxVersion::Text << "\n";
    return 0;
}
