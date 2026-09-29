#pragma once

#include "../CliArguments.h"

class CompileCommand
{
public:
    int execute(const CliArguments& arguments) const;
};
