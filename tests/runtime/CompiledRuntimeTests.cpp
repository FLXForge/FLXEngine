#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace flx::test;

namespace
{
    double localNumber(
        RuntimeObject& object,
        const std::string& key
    )
    {
        const auto it =
            object.local.find(key);

        if (it == object.local.end())
        {
            return 0.0;
        }

        if (const auto* number = std::get_if<double>(&it->second))
        {
            return *number;
        }

        return 0.0;
    }

    void testRuntimeLoadsFromRegistryAfterJsonRemoval()
    {
        const std::filesystem::path root =
            testRoot() / "runtime_registry";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=RuntimeRegistry\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"children\": {\n"
            "    \"ball\": { \"shape\": { \"type\": \"circle\", \"radius\": 2 } },\n"
            "    \"laser\": { \"spawn\": \"manual\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 1, \"height\": 4 } } }\n"
            "  }\n"
            "}\n"
        );

        CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "runtime registry project should compile");

        std::filesystem::remove_all(root / "game");

        RuntimeWorld world;
        ScriptEngine scriptEngine;

        RuntimeLoadResult loadResult =
            world.load(
            result.project,
            scriptEngine
        );

        require(loadResult.success, "runtime should load compiled project");

        RuntimeObject* runtimeRoot =
            world.findByName("root");

        require(runtimeRoot != nullptr, "runtime should create root from registry");
        require(world.findByName("ball") != nullptr, "runtime should create auto child from registry");
        require(runtimeRoot->childResources.count("laser") == 1, "manual child resource should remain available");

        world.spawn(
            *runtimeRoot,
            runtimeRoot->childResources.at("laser"),
            scriptEngine
        );
    }

    void testCompiledScriptsRunWithoutSourceFiles()
    {
        const std::filesystem::path root =
            testRoot() / "compiled_scripts";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "scripts");

        writeFile(
            root / "game.flx",
            "name=CompiledScripts\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"behavior\": { \"scripts\": [\"scripts/root\"] } }\n"
        );

        writeFile(
            root / "game" / "scripts" / "root.js",
            "function born(root) { write_local(root, \"ready\", 1); }\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "script project should compile");
        require(compiled.project.resources.scriptCount() == 1, "script should be embedded");

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "script project should write"
        );

        std::filesystem::remove_all(root / "game");

        CompiledProjectBinaryResult loaded =
            CompiledProjectReader::read(output.generic_string());

        require(loaded.success, "script project should read without sources");

        RuntimeWorld world;
        ScriptEngine scriptEngine;

        RuntimeLoadResult loadResult =
            world.load(
            loaded.project,
            scriptEngine
        );

        require(loadResult.success, "runtime should load compiled script project");

        RuntimeObject* runtimeRoot =
            world.findByName("root");

        require(runtimeRoot != nullptr, "compiled script runtime should create root");
        require(localNumber(*runtimeRoot, "ready") == 1.0, "compiled script should run from embedded source");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "runtime loads from registry after json removal", testRuntimeLoadsFromRegistryAfterJsonRemoval },

        { "compiled scripts run without source files", testCompiledScriptsRunWithoutSourceFiles }

    };

    for (const auto& test : tests)

    {

        try

        {

            test.second();

            std::cout << "[PASS] " << test.first << "\n";

        }

        catch (const std::exception& exception)

        {

            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";

            return 1;

        }

    }

    return 0;

}
