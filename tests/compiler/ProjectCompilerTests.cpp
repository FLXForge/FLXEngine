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
        require(result.project.context.machine.video.screenWidth == 640, "default machine width should be 640");
        require(result.project.context.machine.video.screenHeight == 480, "default machine height should be 480");
    }

    void testExternalMachine()
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
        );

        writeFile(root / "game" / "root.json", "{}\n");

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "project with external machine should compile");
        require(result.project.context.machine.video.screenWidth == 111, "machine width should load");
        require(result.project.context.machine.video.screenHeight == 77, "machine height should load");
        require(result.project.context.machine.video.outputScale == 2, "machine scale should load");
    }

    void testInputMappingIsCompiledFromManifestDirectory()
    {
        const std::filesystem::path root =
            testRoot() / "input_mapping_compile";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");
        std::filesystem::create_directories(root / "input");

        writeFile(
            root / "game.flx",
            "name=InputMapping\n"
            "path=game\n"
            "root=root\n"
            "input.mapping=input/default.input\n"
        );

        writeFile(root / "game" / "root.json", "{}\n");
        writeFile(root / "input" / "default.input", "system.buttons.0=KEY_ESCAPE\n");

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "project with input mapping should compile");
        require(
            result.project.context.inputMappingSourceName == "input/default.input",
            "input mapping source should be relative to manifest directory"
        );
        require(
            result.project.context.inputMappingContent.find("system.buttons.0=KEY_ESCAPE") != std::string::npos,
            "input mapping content should be embedded"
        );
    }

    void testRemovedScreenFieldFails()
    {
        const std::filesystem::path root =
            testRoot() / "removed_screen_field";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=RemovedScreen\n"
            "path=game\n"
            "root=root\n"
            "screen.width=222\n"
        );

        writeFile(root / "game" / "root.json", "{}\n");

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "removed screen field should fail");
        require(result.diagnostics.hasErrors(), "removed screen field should report error");
    }

    void testMissingProject()
    {
        const CompilationResult result =
            compile(testRoot() / "missing.flx");

        require(!result.success, "missing project should fail");
        require(result.diagnostics.hasErrors(), "missing project should report errors");
        require(
            result.diagnostics.all().front().code == DiagnosticCode::ProjectManifestCouldNotBeOpened,
            "missing project should be reported by manifest loader"
        );
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

        { "external machine", testExternalMachine },

        { "input mapping is compiled from manifest directory", testInputMappingIsCompiledFromManifestDirectory },

        { "removed screen field fails", testRemovedScreenFieldFails },

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

