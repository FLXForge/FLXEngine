#pragma once

#include "../CliArguments.h"

class HelpCommand
{
public:
    int execute(const CliArguments& arguments) const;
};
