#include "../support/TestSupport.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

    void testMinimalProject()
    {
        const CompilationResult result =
            compile(createMinimalProject("minimal"));

        require(result.success, "minimal project should compile");
        require(!result.diagnostics.hasErrors(), "minimal project should not have errors");
        require(result.project.rootDefinition.id == "root", "root id should be root");
        require(!result.project.rootId.empty(), "root resource id should be defined");
        require(result.project.resources.objectCount() == 1, "minimal project should register root only");
        require(result.project.resources.findObject(result.project.rootId) != nullptr, "root should be in registry");
    }

    void testDefaultMachine()
    {
        const CompilationResult result =
            compile(createMinimalProject("default_machine"));

        require(result.success, "project without machine should compile");
        require(result.project.context.machinePath.empty(), "machine path should be empty");
        require(result.project.context.screenWidth == 640, "default screen width should be 640");
        require(result.project.context.screenHeight == 480, "default screen height should be 480");
    }

    void testExternalMachineAndScreenOverride()
    {
        const std::filesystem::path root =
            testRoot() / "external_machine";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "machine.yml",
            "machine:\n"
            "  video:\n"
            "    screen:\n"
            "      width: 111\n"
            "      height: 77\n"
            "    output:\n"
            "      scale: 2\n"
        );

        writeFile(
            root / "game.flx",
            "name=MachineTest\n"
            "path=game\n"
            "root=root\n"
            "machine=machine.yml\n"
            "screen.width=222\n"
        );

        writeFile(root / "game" / "root.json", "{}\n");

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "project with external machine should compile");
        require(result.project.context.machine.video.screenWidth == 111, "machine width should load");
        require(result.project.context.screenWidth == 222, "flx screen width should override machine");
        require(result.project.context.screenHeight == 77, "screen height should come from machine");
        require(result.project.context.screenScale == 2, "screen scale should come from machine");
    }

    void testMissingProject()
    {
        const CompilationResult result =
            compile(testRoot() / "missing.flx");

        require(!result.success, "missing project should fail");
        require(result.diagnostics.hasErrors(), "missing project should report errors");
    }

    void testMissingRootJson()
    {
        const std::filesystem::path root =
            testRoot() / "missing_root";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=MissingRoot\n"
            "path=game\n"
            "root=root\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "missing root json should fail");
        require(result.diagnostics.hasErrors(), "missing root json should report errors");
    }

    void testScriptValidation()
    {
        const std::filesystem::path root =
            testRoot() / "script_validation";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=ScriptValidation\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"behavior\": { \"scripts\": [\"scripts/missing\"] } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "missing script should fail compilation");
        require(result.diagnostics.hasErrors(), "missing script should report errors");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "minimal project", testMinimalProject },

        { "default machine", testDefaultMachine },

        { "external machine and screen override", testExternalMachineAndScreenOverride },

        { "missing project", testMissingProject },

        { "missing root json", testMissingRootJson },

        { "script validation", testScriptValidation }

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

