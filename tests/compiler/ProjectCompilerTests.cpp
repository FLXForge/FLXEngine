#include "../support/TestSupport.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    bool hasErrorCode(
        const Diagnostics& diagnostics,
        DiagnosticCode code
    )
    {
        return std::any_of(
            diagnostics.all().begin(),
            diagnostics.all().end(),
            [code](const Diagnostic& diagnostic)
            {
                return
                    diagnostic.severity == DiagnosticSeverity::Error &&
                    diagnostic.code == code;
            }
        );
    }

    const ObjectDefinition& rootObject(const CompiledProject& project)
    {
        const ObjectDefinition* root =
            project.resources.findObject(project.rootId);

        require(root != nullptr, "root should exist in registry");
        return *root;
    }

    void testMinimalProject()
    {
        const CompilationResult result =
            compile(createMinimalProject("minimal"));

        require(result.success, "minimal project should compile");
        require(!result.diagnostics.hasErrors(), "minimal project should not have errors");
        require(!result.project.rootId.empty(), "root resource id should be defined");
        require(result.project.resources.objectCount() == 1, "minimal project should register root only");
        require(result.project.resources.findObject(result.project.rootId) != nullptr, "root should be in registry");
        require(rootObject(result.project).id == "root", "root id should be root");
        require(rootObject(result.project).children.empty(), "compiled root should not keep embedded children");
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

    void testControlPlayerOutsideInputChipFails()
    {
        const std::filesystem::path root =
            testRoot() / "control_player_outside_input_chip";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");
        std::filesystem::create_directories(root / "machines");

        writeFile(
            root / "game.flx",
            "name=ControlPlayerInvalid\n"
            "path=game\n"
            "root=root\n"
            "machine=machines/input.machine.yml\n"
        );

        writeFile(
            root / "machines" / "input.machine.yml",
            "machine:\n"
            "  input:\n"
            "    players:\n"
            "      count: 1\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"control\": { \"player\": 2 }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "control.player outside players count should fail compilation");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::CompErrorUnclassified), "invalid control.player should report a compiler diagnostic");
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

    void writeStateMachineProject(
        const std::string& name,
        const std::string& rootJson
    )
    {
        const std::filesystem::path root =
            testRoot() / name;

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=StateMachineValidation\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            rootJson
        );
    }

    CompilationResult compileStateMachineProject(
        const std::string& name,
        const std::string& rootJson
    )
    {
        writeStateMachineProject(name, rootJson);
        return compile(testRoot() / name / "game.flx");
    }

    void testInvalidStatesTypeFails()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_invalid_type",
                "{ \"states\": [] }\n"
            );

        require(!result.success, "states with non-object type should fail");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::InvalidStateMachineDeclaration), "states type error should use state diagnostic");
    }

    void testMissingInitialStateFails()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_missing_initial",
                "{ \"states\": { \"idle\": {} } }\n"
            );

        require(!result.success, "states without initial should fail");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::MissingStateMachineInitialState), "missing initial should use state diagnostic");
    }

    void testEmptyInitialStateFails()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_empty_initial",
                "{ \"states\": { \"initial\": \"\", \"idle\": {} } }\n"
            );

        require(!result.success, "empty initial state should fail");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::MissingStateMachineInitialState), "empty initial should use state diagnostic");
    }

    void testInitialStateMustBeDeclared()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_initial_missing",
                "{ \"states\": { \"initial\": \"idle\", \"other\": {} } }\n"
            );

        require(!result.success, "initial state must refer to a declared state");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::MissingStateMachineState), "missing initial declaration should use state diagnostic");
    }

    void testInvalidStateEntryFails()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_entry_invalid",
                "{ \"states\": { \"initial\": \"idle\", \"idle\": true } }\n"
            );

        require(!result.success, "state entries must be objects");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::InvalidStateMachineDeclaration), "invalid state entry should use state diagnostic");
    }

    void testInvalidNextDeclarationFails()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_next_invalid",
                "{ \"states\": { \"initial\": \"idle\", \"idle\": { \"next\": \"play\" }, \"play\": {} } }\n"
            );

        require(!result.success, "next must be an array");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::InvalidStateMachineDeclaration), "invalid next should use state diagnostic");
    }

    void testMissingNextTargetFails()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_next_missing",
                "{ \"states\": { \"initial\": \"idle\", \"idle\": { \"next\": [\"play\"] } } }\n"
            );

        require(!result.success, "next targets must exist");
        require(hasErrorCode(result.diagnostics, DiagnosticCode::MissingStateMachineState), "missing next target should use state diagnostic");
    }

    void testValidSingleTerminalStateCompiles()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_single_terminal",
                "{ \"states\": { \"initial\": \"idle\", \"idle\": {} } }\n"
            );

        require(result.success, "single terminal state should compile");
        require(!result.diagnostics.hasErrors(), "single terminal state should not report errors");
    }

    void testValidStateCyclesCompile()
    {
        const CompilationResult result =
            compileStateMachineProject(
                "state_valid_cycles",
                "{\n"
                "  \"states\": {\n"
                "    \"initial\": \"a\",\n"
                "    \"a\": { \"next\": [\"b\"] },\n"
                "    \"b\": { \"next\": [\"a\", \"b\"] }\n"
                "  }\n"
                "}\n"
            );

        require(result.success, "state cycles and explicit self-transition should compile");
        require(!result.diagnostics.hasErrors(), "valid state cycles should not report errors");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "minimal project", testMinimalProject },

        { "default machine", testDefaultMachine },

        { "external machine", testExternalMachine },

        { "input mapping is compiled from manifest directory", testInputMappingIsCompiledFromManifestDirectory },

        { "control player outside input chip fails", testControlPlayerOutsideInputChipFails },

        { "removed screen field fails", testRemovedScreenFieldFails },

        { "missing project", testMissingProject },

        { "missing root json", testMissingRootJson },

        { "script validation", testScriptValidation },

        { "invalid states type fails", testInvalidStatesTypeFails },

        { "missing initial state fails", testMissingInitialStateFails },

        { "empty initial state fails", testEmptyInitialStateFails },

        { "initial state must be declared", testInitialStateMustBeDeclared },

        { "invalid state entry fails", testInvalidStateEntryFails },

        { "invalid next declaration fails", testInvalidNextDeclarationFails },

        { "missing next target fails", testMissingNextTargetFails },

        { "valid single terminal state compiles", testValidSingleTerminalStateCompiles },

        { "valid state cycles compile", testValidStateCyclesCompile }

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

