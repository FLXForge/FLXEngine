#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

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

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "compiled project roundtrip minimal", testCompiledProjectRoundTripMinimal },

        { "compiled project roundtrip graph", testCompiledProjectRoundTripGraph },

        { "invalid compiled magic", testInvalidCompiledMagic },

        { "invalid compiled version", testInvalidCompiledVersion },

        { "truncated compiled project", testTruncatedCompiledProject }

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

