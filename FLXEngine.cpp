#include "engine/core/Engine.h"
#include "engine/debug/Logger.h"

#include <iostream>

inline constexpr const char* FLXENGINE_VERSION = "0.2.0";

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        Logger::error("run", "Should define FLX proyect 'FLXEngine.exe [project.flx path]'");
        return 1;
    }

    Logger::info("engine", std::string("FLXEngine version ") + FLXENGINE_VERSION);

    Engine engine;
    engine.run(argv[1]);
    return 0;
}