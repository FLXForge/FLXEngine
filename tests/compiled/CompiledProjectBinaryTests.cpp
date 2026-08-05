#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    const ObjectDefinition& rootObject(const CompiledProject& project)
    {
        const ObjectDefinition* root =
            project.resources.findObject(project.rootId);

        require(root != nullptr, "root should exist in registry");
        return *root;
    }

    std::vector<unsigned char> readBinaryFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::vector<unsigned char>(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }

    void replaceFirstAscii(
        std::vector<unsigned char>& bytes,
        const std::string& from,
        const std::string& to
    )
    {
        require(from.size() == to.size(), "replacement must keep binary size");

        auto it =
            std::search(
                bytes.begin(),
                bytes.end(),
                from.begin(),
                from.end()
            );

        require(it != bytes.end(), "binary text to replace should exist");

        std::copy(
            to.begin(),
            to.end(),
            it
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
        require(loaded.project.resources.objectCount() == 1, "minimal flxc should not duplicate root definition");
        require(rootObject(loaded.project).children.empty(), "compiled root should not keep embedded children");
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
        require(rootObject(loaded.project).children.empty(), "compiled graph root should not keep embedded children");
        require(rootObject(loaded.project).childResources.count("brick") == 1, "brick resource relation should survive");
        require(rootObject(loaded.project).childResources.count("laser") == 1, "manual resource relation should survive");
        require(rootObject(loaded.project).creationMode == "grid", "grid creation mode should survive");
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

    void testMissingRootIdFailsRead()
    {
        const std::filesystem::path flxPath =
            createMinimalProject("missing_root_id_binary");

        CompilationResult compiled =
            compile(flxPath);

        require(compiled.success, "project should compile before root id mutation");

        const std::filesystem::path output =
            testRoot() / "missing_root_id_binary" / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "compiled project should write before root id mutation"
        );

        std::vector<unsigned char> bytes =
            readBinaryFile(output);

        const std::string replacement(
            compiled.project.rootId.size(),
            'x'
        );

        replaceFirstAscii(
            bytes,
            compiled.project.rootId,
            replacement
        );

        const std::filesystem::path broken =
            testRoot() / "missing_root_id_binary" / "broken.flxc";

        writeBinary(
            broken,
            bytes
        );

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(broken.generic_string());

        require(!result.success, "binary with missing root id should fail");
        require(result.diagnostics.hasErrors(), "missing root id should report diagnostics");
    }

    void testBrokenChildResourcesFailWrite()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.childResources["missing"] = "missing";

        require(project.resources.addObject(project.rootId, root), "root should register");

        Diagnostics diagnostics;

        require(
            !CompiledProjectWriter::write(
                (testRoot() / "broken_child_resource" / "game.flxc").generic_string(),
                project,
                diagnostics
            ),
            "writer should reject missing child resource"
        );

        require(diagnostics.hasErrors(), "broken child resource should report diagnostics");
    }

    void testEmbeddedChildrenFailWrite()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.children["child"].id = "child";

        require(project.resources.addObject(project.rootId, root), "root should register");

        Diagnostics diagnostics;

        require(
            !CompiledProjectWriter::write(
                (testRoot() / "embedded_children" / "game.flxc").generic_string(),
                project,
                diagnostics
            ),
            "writer should reject embedded children"
        );

        require(diagnostics.hasErrors(), "embedded children should report diagnostics");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "compiled project roundtrip minimal", testCompiledProjectRoundTripMinimal },

        { "compiled project roundtrip graph", testCompiledProjectRoundTripGraph },

        { "invalid compiled magic", testInvalidCompiledMagic },

        { "invalid compiled version", testInvalidCompiledVersion },

        { "truncated compiled project", testTruncatedCompiledProject },

        { "missing root id fails read", testMissingRootIdFailsRead },

        { "broken childResources fail write", testBrokenChildResourcesFailWrite },

        { "embedded children fail write", testEmbeddedChildrenFailWrite }

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

