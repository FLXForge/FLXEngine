#include "../support/TestSupport.h"
#include "TemporaryDirectory.h"
#include <compiler/CompiledProjectBinary.h>
#include <core/FlxSession.h>
#include <persistence/PersistenceSystem.h>
#include <fstream>
#include <iostream>

using namespace flx::test;
using namespace flx::portability;

namespace
{
    CompilationResult compileProject(const std::filesystem::path& path)
    {
        auto result = compile(path);
        if (!result.success)
            for (const auto& diagnostic : result.diagnostics.all())
                std::cerr << diagnosticCodeText(diagnostic.code) << " " << diagnostic.message << '\n';
        return result;
    }

    void resourceResolution(const std::filesystem::path& directory)
    {
        const auto project = directory / "project with spaces";
        writeFile(project / "game.flx",
            "name=PathTest\r\npath=world\r\nroot=game\r\nmachine=machine.yml\r\n"
            "input.mapping=input/default.input\r\n");
        writeFile(project / "machine.yml",
            "machine:\n  video:\n    screen:\n      width: 64\n      height: 64\n");
        writeFile(project / "input/default.input", "system.buttons.0=KEY_ESCAPE\n");
        writeFile(project / "world/game.json",
            R"({"delimit":true,"origin":{"x":32,"y":32},"size":{"width":64,"height":64},
                "children":{"marker":{"like":"parts\\marker"}}})");
        writeFile(project / "world/parts/marker.json",
            R"({"behavior":{"scripts":["/scripts\\entry"]}})");
        writeFile(project / "world/scripts/entry.js",
            "function born(o){save('paths','ready',true);}");
        std::filesystem::create_directory(directory / "different cwd");
        WorkingDirectory cwd(directory / "different cwd");
        auto absolute = compileProject(project / "game.flx");
        require(absolute.success, "absolute manifest resolves resources from the project, not cwd");
        auto relative = compileProject(std::filesystem::path("../project with spaces/game.flx"));
        require(relative.success, "relative manifest with spaces");
        require(absolute.project.context.machine.video.screenWidth == 64 &&
            absolute.project.resources.scriptCount() == 1, "external machine, mapping and script were loaded");
        require(relative.project.resources.objectCount() == absolute.project.resources.objectCount() &&
            relative.project.resources.scriptCount() == absolute.project.resources.scriptCount(),
            "absolute and relative entry points resolve equivalent resources");
        FlxSessionOptions options;
        options.enableVideo = options.enableAudio = false;
        options.savePath = "saved games/nested";
        FlxSession session;
        require(session.load(absolute.project, options), "execute compiled references");
        session.unload();
        PersistenceSystem saves(directory / "different cwd/saved games/nested");
        auto ready = saves.load("paths", "ready");
        require(ready && ready->booleanValue, "referenced script executed and saved at the selected path");
        require(!std::filesystem::exists(project / "saves"), "no implicit project save location");
        require(!compile(project / "missing.flx").success, "missing manifest is diagnosed");
        require(!compile(project / "absent directory/game.flx").success, "missing directory is diagnosed");
        writeFile(project / "world/game.json", R"({"children":{"marker":{"like":"parts/missing"}}})");
        require(!compile(project / "game.flx").success, "missing resource is diagnosed");
    }

    void caseSensitivity(const std::filesystem::path& directory)
    {
        const auto project = directory / "case";
        writeFile(project / "game.FLX", "name=Case\npath=world\nroot=game\n");
        writeFile(project / "world/game.json", R"({"children":{"marker":{"like":"Marker"}}})");
        writeFile(project / "world/Marker.json", "{}");
        require(compile(project / "game.FLX").success, "existing uppercase manifest extension");
        const bool sensitive = !std::filesystem::exists(project / "world/marker.json");
        writeFile(project / "world/game.json", R"({"children":{"marker":{"like":"marker"}}})");
        const auto wrongCase = compile(project / "game.FLX");
        require(wrongCase.success == !sensitive,
            "resource lookup follows the actual filesystem case policy");
        std::cout << "[PASS] filesystem case sensitivity=" << sensitive << '\n';
    }
}

int main()
{
    try
    {
        TemporaryDirectory temporary;
        resourceResolution(temporary.path());
        caseSensitivity(temporary.path());
        std::cout << "[PASS] portable paths, separators, cwd, spaces, missing resources and savePath\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
