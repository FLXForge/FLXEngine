#pragma once

#include "FlxContext.h"

#include <string>

class FlxContextBuilder
{
public:
    static FlxContext build(const std::string& path);

private:
    static void assign(
        FlxContext& context,
        const std::string& key,
        const std::string& value
    );

    static std::string trim(const std::string& value);
    static std::string getDirectory(const std::string& path);
    static std::string joinPath(
        const std::string& basePath,
        const std::string& childPath
    );
    static bool parseBool(const std::string& value);
};
