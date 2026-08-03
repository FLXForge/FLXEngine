#include "../engine/compiler/ProjectCompiler.h"
#include "../engine/compiler/CompiledProjectBinary.h"
#include "../engine/cli/CliParser.h"
#include "../engine/cli/ProjectResolver.h"
#include "../engine/runtime/RuntimeWorld.h"
#include "../engine/scripting/ScriptEngine.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::filesystem::path testRoot()
    {
        return
            std::filesystem::temp_directory_path() /
            "flx_project_compiler_tests";
    }

    void writeFile(
        const std::filesystem::path& path,
        const std::string& text
    )
    {
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path);
        file << text;
    }

    void writeBinary(
        const std::filesystem::path& path,
        const std::vector<unsigned char>& data
    )
    {
        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary);
        file.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size())
        );
    }

    void require(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    CompilationResult compile(const std::filesystem::path& path)
    {
        ProjectCompiler compiler;
        return compiler.compile(path.generic_string());
    }

    CliParseResult parseArguments(const std::vector<std::string>& arguments)
    {
        std::vector<std::string> values;
        values.emplace_back("flx");
        values.insert(values.end(), arguments.begin(), arguments.end());

        std::vector<char*> argv;

        for (std::string& value : values)
        {
            argv.push_back(value.data());
        }

        CliParser parser;
        return parser.parse(
            static_cast<int>(argv.size()),
            argv.data()
        );
    }

    std::filesystem::path createMinimalProject(const std::string& name)
    {
        const std::filesystem::path root =
            testRoot() / name;

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Test\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{}\n"
        );

        return root / "game.flx";
    }

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

    void testGraphWithAutoAndManualChildren()
    {
        const std::filesystem::path root =
            testRoot() / "graph_children";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Graph\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"children\": {\n"
            "    \"ship\": { \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 8 } } },\n"
            "    \"laser\": { \"spawn\": \"manual\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 1, \"height\": 4 } } }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "graph with children should compile");
        require(result.project.resources.objectCount() == 3, "root and two children should be registered");
        require(result.project.rootDefinition.childResources.count("ship") == 1, "auto child should have resource id");
        require(result.project.rootDefinition.childResources.count("laser") == 1, "manual child should have resource id");
    }

    void testGraphWithGridChildren()
    {
        const std::filesystem::path root =
            testRoot() / "graph_grid";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=Grid\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"creation\": {\n"
            "    \"mode\": \"grid\",\n"
            "    \"rules\": { \"rows\": 1, \"columns\": 2, \"cellWidth\": 8, \"cellHeight\": 8 },\n"
            "    \"pattern\": [\"brick\"]\n"
            "  },\n"
            "  \"children\": {\n"
            "    \"brick\": { \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 8 } } }\n"
            "  }\n"
            "}\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "grid graph should compile");
        require(result.project.resources.objectCount() == 2, "grid child should be registered");
        require(result.project.rootDefinition.childResources.count("brick") == 1, "grid child should have resource id");
    }

    void testGraphWithLike()
    {
        const std::filesystem::path root =
            testRoot() / "graph_like";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "objects");

        writeFile(
            root / "game.flx",
            "name=Like\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"children\": { \"brick\": { \"like\": \"objects/brick\", \"role\": \"strong\" } } }\n"
        );

        writeFile(
            root / "game" / "objects" / "brick.json",
            "{ \"group\": \"brick\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 4 } } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "like graph should compile");
        const std::string childId =
            result.project.rootDefinition.childResources.at("brick");
        const ObjectDefinition* child =
            result.project.resources.findObject(childId);

        require(child != nullptr, "liked child should be registered");
        require(child->group == "brick", "liked child should inherit group");
        require(child->role == "strong", "liked child should keep override");
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

    void testValidFlxReference()
    {
        const std::filesystem::path root =
            testRoot() / "valid_reference";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "blocks");

        writeFile(
            root / "game.flx",
            "name=Reference\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"shape\": \"/blocks/shapes:dot\" }\n"
        );

        writeFile(
            root / "game" / "blocks" / "shapes.json",
            "{ \"dot\": { \"type\": \"circle\", \"radius\": 3, \"color\": \"white\" } }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(result.success, "valid FLX reference should compile");
        require(result.project.rootDefinition.shapeType == "circle", "referenced shape should resolve");
        require(result.project.rootDefinition.radius == 3.0f, "referenced radius should resolve");
    }

    void testInvalidFlxReference()
    {
        const std::filesystem::path root =
            testRoot() / "invalid_reference";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=InvalidReference\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"shape\": \"/missing:block\" }\n"
        );

        const CompilationResult result =
            compile(root / "game.flx");

        require(!result.success, "invalid FLX reference should fail");
        require(result.diagnostics.hasErrors(), "invalid FLX reference should report errors");
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

        world.load(
            result.project,
            scriptEngine
        );

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

    void testCompiledProjectRoundTripMinimal()
    {
        const std::filesystem::path flxPath =
            createMinimalProject("roundtrip_minimal");

        CompilationResult compiled =
            compile(flxPath);

        require(compiled.success, "minimal project should compile before roundtrip");

        const std::filesystem::path output =
            testRoot() / "roundtrip_minimal" / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "compiled project should write"
        );

        CompiledProjectBinaryResult loaded =
            CompiledProjectReader::read(output.generic_string());

        require(loaded.success, "compiled project should read");
        require(loaded.project.context.name == compiled.project.context.name, "context name should survive roundtrip");
        require(loaded.project.rootId == compiled.project.rootId, "root id should survive roundtrip");
        require(loaded.project.resources.objectCount() == compiled.project.resources.objectCount(), "object count should survive roundtrip");
    }

    void testCompiledProjectRoundTripGraph()
    {
        const std::filesystem::path root =
            testRoot() / "roundtrip_graph";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=RoundTripGraph\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"creation\": {\n"
            "    \"mode\": \"grid\",\n"
            "    \"rules\": { \"rows\": 1, \"columns\": 2, \"cellWidth\": 8, \"cellHeight\": 8 },\n"
            "    \"pattern\": [\"brick\"]\n"
            "  },\n"
            "  \"children\": {\n"
            "    \"brick\": { \"shape\": { \"type\": \"block\", \"size\": { \"width\": 8, \"height\": 8 } } },\n"
            "    \"laser\": { \"spawn\": \"manual\", \"shape\": { \"type\": \"block\", \"size\": { \"width\": 1, \"height\": 4 } } }\n"
            "  }\n"
            "}\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "graph project should compile before roundtrip");

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "graph project should write"
        );

        CompiledProjectBinaryResult loaded =
            CompiledProjectReader::read(output.generic_string());

        require(loaded.success, "graph project should read");
        require(loaded.project.rootDefinition.childResources.count("brick") == 1, "brick resource relation should survive");
        require(loaded.project.rootDefinition.childResources.count("laser") == 1, "manual resource relation should survive");
        require(loaded.project.rootDefinition.creationMode == "grid", "grid creation mode should survive");
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
            "function born(root) { root.local[\"ready\"] = 1; }\n"
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

        world.load(
            loaded.project,
            scriptEngine
        );

        RuntimeObject* runtimeRoot =
            world.findByName("root");

        require(runtimeRoot != nullptr, "compiled script runtime should create root");
        require(runtimeRoot->local["ready"] == 1.0, "compiled script should run from embedded source");
    }

    void testCompiledExamplesRoundTrip()
    {
        const std::filesystem::path sourceRoot =
            std::filesystem::path(FLX_SOURCE_DIR);

        const std::vector<std::string> examples = {
            "pong",
            "asteroids",
            "arkanoid",
            "invaders"
        };

        for (const std::string& example : examples)
        {
            CompilationResult compiled =
                compile(sourceRoot / "examples" / (example + ".flx"));

            require(
                compiled.success,
                "known example should compile before compiled roundtrip: " + example
            );

            const std::filesystem::path output =
                testRoot() / "compiled_examples" / (example + ".flxc");

            Diagnostics writeDiagnostics;

            require(
                CompiledProjectWriter::write(
                    output.generic_string(),
                    compiled.project,
                    writeDiagnostics
                ),
                "known example should write compiled file: " + example
            );

            CompiledProjectBinaryResult loaded =
                CompiledProjectReader::read(output.generic_string());

            require(
                loaded.success,
                "known example should read compiled file: " + example
            );

            require(
                loaded.project.resources.objectCount() == compiled.project.resources.objectCount(),
                "known example object count should survive compiled roundtrip: " + example
            );
        }
    }

    void testInvalidCompiledMagic()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "bad_magic.flxc";

        writeBinary(
            path,
            { 0, 0, 0, 0, 1, 0, 0, 0 }
        );

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "invalid magic should fail");
        require(result.diagnostics.hasErrors(), "invalid magic should report errors");
    }

    void testInvalidCompiledVersion()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "bad_version.flxc";

        writeBinary(
            path,
            { 'F', 'L', 'X', 'C', 99, 0, 0, 0 }
        );

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "invalid version should fail");
        require(result.diagnostics.hasErrors(), "invalid version should report errors");
    }

    void testTruncatedCompiledProject()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "truncated.flxc";

        writeBinary(
            path,
            { 'F', 'L', 'X', 'C', 1, 0 }
        );

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "truncated compiled project should fail");
        require(result.diagnostics.hasErrors(), "truncated compiled project should report errors");
    }

    void testKnownExamples()
    {
        const std::filesystem::path sourceRoot =
            std::filesystem::path(FLX_SOURCE_DIR);

        const std::vector<std::string> examples = {
            "pong",
            "asteroids",
            "arkanoid",
            "invaders"
        };

        for (const std::string& example : examples)
        {
            const CompilationResult result =
                compile(sourceRoot / "examples" / (example + ".flx"));

            require(
                result.success,
                "known example should compile: " + example
            );

            require(
                !result.project.rootDefinition.id.empty(),
                "known example should produce root definition: " + example
            );
        }
    }

    void testCliParserDefaults()
    {
        CliParseResult result =
            parseArguments({});

        require(result.success, "empty CLI should parse");
        require(result.arguments.command == CliCommand::Run, "empty CLI should default to run");
        require(result.arguments.target == ".", "empty CLI should default target to current directory");

        result =
            parseArguments({ "." });

        require(result.success, "directory target should parse");
        require(result.arguments.command == CliCommand::Run, "directory target should imply run");
        require(result.arguments.target == ".", "directory target should be preserved");

        result =
            parseArguments({ "project.flx" });

        require(result.success, ".flx target should parse");
        require(result.arguments.command == CliCommand::Run, ".flx target should imply run");
        require(result.arguments.target == "project.flx", ".flx target should be preserved");

        result =
            parseArguments({ "run" });

        require(result.success, "run without target should parse");
        require(result.arguments.command == CliCommand::Run, "run command should parse");
        require(result.arguments.target == ".", "run should default target to current directory");
    }

    void testCliParserCommands()
    {
        CliParseResult result =
            parseArguments({ "run", "--frames=10", "." });

        require(result.success, "run with frames should parse");
        require(result.arguments.command == CliCommand::Run, "run command should be selected");
        require(result.arguments.maxFrames == 10, "frames should parse");

        result =
            parseArguments({ "compile", "--output=game.flxc", "." });

        require(result.success, "compile with output should parse");
        require(result.arguments.command == CliCommand::Compile, "compile command should be selected");
        require(result.arguments.output->generic_string() == "game.flxc", "compile output should parse");

        result =
            parseArguments({ "compile", "-o", "game.flxc", "." });

        require(result.success, "compile with -o should parse");
        require(result.arguments.output->generic_string() == "game.flxc", "-o output should parse");

        result =
            parseArguments({ "run-compiled", "game.flxc" });

        require(result.success, "run-compiled should parse");
        require(result.arguments.command == CliCommand::RunCompiled, "run-compiled command should be selected");
        require(result.arguments.target == "game.flxc", "run-compiled target should parse");

        result =
            parseArguments({ "validate", "." });

        require(result.success, "validate should parse");
        require(result.arguments.command == CliCommand::Validate, "validate command should be selected");
    }

    void testCliParserHelpAndVersion()
    {
        CliParseResult result =
            parseArguments({ "--version" });

        require(result.success, "--version should parse");
        require(result.arguments.command == CliCommand::Version, "--version should select version");

        result =
            parseArguments({ "-v" });

        require(result.success, "-v should parse");
        require(result.arguments.command == CliCommand::Version, "-v should select version");

        result =
            parseArguments({ "version" });

        require(result.success, "version command should parse");
        require(result.arguments.command == CliCommand::Version, "version command should be selected");

        result =
            parseArguments({ "--help" });

        require(result.success, "--help should parse");
        require(result.arguments.command == CliCommand::Help, "--help should select help");

        result =
            parseArguments({ "help", "run" });

        require(result.success, "help run should parse");
        require(result.arguments.command == CliCommand::Help, "help command should be selected");
        require(result.arguments.helpCommand == CliCommand::Run, "help run should select run help");

        result =
            parseArguments({ "run", "--help" });

        require(result.success, "run --help should parse");
        require(result.arguments.command == CliCommand::Help, "run --help should select help");
        require(result.arguments.helpCommand == CliCommand::Run, "run --help should select run help");
    }

    void testCliParserInvalidArguments()
    {
        CliParseResult result =
            parseArguments({ "unknown" });

        require(!result.success, "unknown command should fail");
        require(result.exitCode == CliExitCode::InvalidArguments, "unknown command should return invalid arguments");

        result =
            parseArguments({ "run", "--frames=abc", "." });

        require(!result.success, "non numeric frames should fail");

        result =
            parseArguments({ "run", "--frames=0", "." });

        require(!result.success, "zero frames should fail");

        result =
            parseArguments({ "run", "--frames=-1", "." });

        require(!result.success, "negative frames should fail");

        result =
            parseArguments({ "compile", "." });

        require(!result.success, "compile without output should fail");

        result =
            parseArguments({ "compile", "--output=a.flxc", "--output=b.flxc", "." });

        require(!result.success, "duplicate output should fail");
    }

    void testCliProjectResolver()
    {
        const std::filesystem::path root =
            testRoot() / "cli_resolver";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);

        writeFile(root / "direct.flx", "name=Direct\npath=game\nroot=root\n");

        ProjectResolver resolver;
        ProjectResolutionResult result =
            resolver.resolve(root / "direct.flx");

        require(result.success, "direct .flx should resolve");
        require(result.manifestPath.filename() == "direct.flx", "direct .flx should be returned");

        const std::filesystem::path projectDir =
            root / "project_dir";

        std::filesystem::create_directories(projectDir);
        writeFile(projectDir / "project.flx", "name=Project\npath=game\nroot=root\n");

        result =
            resolver.resolve(projectDir);

        require(result.success, "directory with project.flx should resolve");
        require(result.manifestPath.filename() == "project.flx", "project.flx should be preferred");

        const std::filesystem::path singleDir =
            root / "single_dir";

        std::filesystem::create_directories(singleDir);
        writeFile(singleDir / "single.flx", "name=Single\npath=game\nroot=root\n");

        result =
            resolver.resolve(singleDir);

        require(result.success, "directory with one .flx should resolve");
        require(result.manifestPath.filename() == "single.flx", "single .flx should be selected");

        const std::filesystem::path emptyDir =
            root / "empty_dir";

        std::filesystem::create_directories(emptyDir);

        result =
            resolver.resolve(emptyDir);

        require(!result.success, "directory without .flx should fail");

        const std::filesystem::path ambiguousDir =
            root / "ambiguous_dir";

        std::filesystem::create_directories(ambiguousDir);
        writeFile(ambiguousDir / "a.flx", "name=A\npath=game\nroot=root\n");
        writeFile(ambiguousDir / "b.flx", "name=B\npath=game\nroot=root\n");

        result =
            resolver.resolve(ambiguousDir);

        require(!result.success, "directory with multiple .flx files should fail");

        result =
            resolver.resolve(root / "missing");

        require(!result.success, "missing target should fail");
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
        { "graph with auto and manual children", testGraphWithAutoAndManualChildren },
        { "graph with grid children", testGraphWithGridChildren },
        { "graph with like", testGraphWithLike },
        { "valid FLX reference", testValidFlxReference },
        { "invalid FLX reference", testInvalidFlxReference },
        { "script validation", testScriptValidation },
        { "runtime loads from registry after json removal", testRuntimeLoadsFromRegistryAfterJsonRemoval },
        { "compiled project roundtrip minimal", testCompiledProjectRoundTripMinimal },
        { "compiled project roundtrip graph", testCompiledProjectRoundTripGraph },
        { "compiled scripts run without source files", testCompiledScriptsRunWithoutSourceFiles },
        { "compiled examples roundtrip", testCompiledExamplesRoundTrip },
        { "invalid compiled magic", testInvalidCompiledMagic },
        { "invalid compiled version", testInvalidCompiledVersion },
        { "truncated compiled project", testTruncatedCompiledProject },
        { "known examples", testKnownExamples },
        { "CLI parser defaults", testCliParserDefaults },
        { "CLI parser commands", testCliParserCommands },
        { "CLI parser help and version", testCliParserHelpAndVersion },
        { "CLI parser invalid arguments", testCliParserInvalidArguments },
        { "CLI project resolver", testCliProjectResolver }
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

