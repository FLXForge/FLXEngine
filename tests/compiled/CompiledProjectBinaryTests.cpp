#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/compiler/binary/BinaryLimits.h"
#include <algorithm>
#include <cstdint>
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

    void replaceAllAscii(
        std::vector<unsigned char>& bytes,
        const std::string& from,
        const std::string& to
    )
    {
        require(from.size() == to.size(), "replacement must keep binary size");

        auto searchFrom =
            bytes.begin();

        bool replaced = false;

        while (true)
        {
            auto it =
                std::search(
                    searchFrom,
                    bytes.end(),
                    from.begin(),
                    from.end()
                );

            if (it == bytes.end())
            {
                break;
            }

            std::copy(
                to.begin(),
                to.end(),
                it
            );

            searchFrom =
                it + static_cast<std::ptrdiff_t>(to.size());
            replaced = true;
        }

        require(replaced, "binary text to replace should exist");
    }

    void appendU8(
        std::vector<unsigned char>& bytes,
        uint8_t value
    )
    {
        bytes.push_back(value);
    }

    void appendU32(
        std::vector<unsigned char>& bytes,
        uint32_t value
    )
    {
        bytes.push_back(static_cast<unsigned char>(value & 0xffu));
        bytes.push_back(static_cast<unsigned char>((value >> 8) & 0xffu));
        bytes.push_back(static_cast<unsigned char>((value >> 16) & 0xffu));
        bytes.push_back(static_cast<unsigned char>((value >> 24) & 0xffu));
    }

    void appendI32(
        std::vector<unsigned char>& bytes,
        int32_t value
    )
    {
        appendU32(
            bytes,
            static_cast<uint32_t>(value)
        );
    }

    void appendString(
        std::vector<unsigned char>& bytes,
        const std::string& value
    )
    {
        appendU32(
            bytes,
            static_cast<uint32_t>(value.size())
        );

        bytes.insert(
            bytes.end(),
            value.begin(),
            value.end()
        );
    }

    void appendMinimalHeaderAndContext(
        std::vector<unsigned char>& bytes
    )
    {
        appendU32(bytes, 0x43584C46u);
        appendU32(bytes, 3u);
        appendString(bytes, "test-producer");

        for (int i = 0; i < 7; ++i)
        {
            appendString(bytes, "");
        }

        appendI32(bytes, 640);
        appendI32(bytes, 480);
        appendString(bytes, "black");
    }

    bool hasDiagnosticCode(
        const Diagnostics& diagnostics,
        DiagnosticCode code
    )
    {
        return std::any_of(
            diagnostics.all().begin(),
            diagnostics.all().end(),
            [code](const Diagnostic& diagnostic)
            {
                return diagnostic.code == code;
            }
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
        require(loaded.metadata.formatVersion == 3, "format version should be exposed");
        require(!loaded.metadata.producerVersion.empty(), "producer version should be exposed");
        require(loaded.project.context.name == compiled.project.context.name, "context name should survive roundtrip");
        require(loaded.project.rootId == compiled.project.rootId, "root id should survive roundtrip");
        require(loaded.project.resources.objectCount() == compiled.project.resources.objectCount(), "object count should survive roundtrip");
        require(loaded.project.resources.objectCount() == 1, "minimal flxc should not duplicate root definition");
        require(rootObject(loaded.project).children.empty(), "compiled root should not keep embedded children");
    }

    void testDeterministicBytes()
    {
        const std::filesystem::path flxPath =
            createMinimalProject("deterministic_bytes");

        CompilationResult first =
            compile(flxPath);
        CompilationResult second =
            compile(flxPath);

        require(first.success, "first project compile should succeed");
        require(second.success, "second project compile should succeed");

        const std::filesystem::path firstOutput =
            testRoot() / "deterministic_bytes" / "first.flxc";
        const std::filesystem::path secondOutput =
            testRoot() / "deterministic_bytes" / "second.flxc";

        Diagnostics firstDiagnostics;
        Diagnostics secondDiagnostics;

        require(CompiledProjectWriter::write(firstOutput.generic_string(), first.project, firstDiagnostics), "first binary should write");
        require(CompiledProjectWriter::write(secondOutput.generic_string(), second.project, secondDiagnostics), "second binary should write");
        require(readBinaryFile(firstOutput) == readBinaryFile(secondOutput), "identical compiles should produce identical bytes");
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
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::InvalidCompiledProjectMagic), "invalid magic should use stable code");
    }

    void testInvalidCompiledVersion()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "bad_version.flxc";

        writeBinary(
            path,
            { 'F', 'L', 'X', 'C', 2, 0, 0, 0 }
        );

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "invalid version should fail");
        require(result.diagnostics.hasErrors(), "invalid version should report errors");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::UnsupportedCompiledProjectFormat), "v2 should be rejected explicitly");
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
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::TruncatedCompiledProject), "truncated file should use stable code");
    }

    void testMissingFile()
    {
        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(
                (testRoot() / "missing" / "missing.flxc").generic_string()
            );

        require(!result.success, "missing file should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectCouldNotBeOpened), "missing file should use open diagnostic");
    }

    void testStringLimitExceeded()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "string_limit.flxc";

        std::vector<unsigned char> bytes;
        appendU32(bytes, 0x43584C46u);
        appendU32(bytes, 3u);
        appendU32(bytes, flx::binary::MaxStringSize + 1u);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "string limit should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "string limit should use limit diagnostic");
    }

    void testCollectionLimitExceeded()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "collection_limit.flxc";

        std::vector<unsigned char> bytes;
        appendMinimalHeaderAndContext(bytes);
        appendU8(bytes, 0);
        appendU32(bytes, flx::binary::MaxCollectionCount + 1u);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "collection limit should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "collection limit should use limit diagnostic");
    }

    void testInvalidBool()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "invalid_bool.flxc";

        std::vector<unsigned char> bytes;
        appendMinimalHeaderAndContext(bytes);
        appendU8(bytes, 2);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "invalid bool should fail");
        require(result.diagnostics.hasErrors(), "invalid bool should report diagnostics");
    }

    void testTrailingBytes()
    {
        const std::filesystem::path flxPath =
            createMinimalProject("trailing_bytes");

        CompilationResult compiled =
            compile(flxPath);

        require(compiled.success, "project should compile before trailing mutation");

        const std::filesystem::path output =
            testRoot() / "trailing_bytes" / "game.flxc";

        Diagnostics writeDiagnostics;

        require(CompiledProjectWriter::write(output.generic_string(), compiled.project, writeDiagnostics), "compiled project should write");

        std::vector<unsigned char> bytes =
            readBinaryFile(output);

        bytes.push_back(0x7fu);

        const std::filesystem::path broken =
            testRoot() / "trailing_bytes" / "broken.flxc";

        writeBinary(broken, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(broken.generic_string());

        require(!result.success, "trailing bytes should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::TrailingCompiledProjectData), "trailing bytes should use stable code");
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
        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompErrorUnclassified), "model invariant should use COMP diagnostic");
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
        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompErrorUnclassified), "embedded children should use COMP diagnostic");
    }

    void testMissingScriptFailsWrite()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.resolvedScriptPaths.push_back("missing-script");

        require(project.resources.addObject(project.rootId, root), "root should register");

        Diagnostics diagnostics;

        require(
            !CompiledProjectWriter::write(
                (testRoot() / "missing_script" / "game.flxc").generic_string(),
                project,
                diagnostics
            ),
            "writer should reject missing script"
        );

        require(diagnostics.hasErrors(), "missing script should report diagnostics");
        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompErrorUnclassified), "missing script should use COMP diagnostic");
    }

    void testNoPartialFileOnWriteFailure()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.children["child"].id = "child";

        require(project.resources.addObject(project.rootId, root), "root should register");

        const std::filesystem::path output =
            testRoot() / "partial_write" / "game.flxc";

        writeBinary(
            output,
            { 'o', 'l', 'd' }
        );

        Diagnostics diagnostics;

        require(
            !CompiledProjectWriter::write(
                output.generic_string(),
                project,
                diagnostics
            ),
            "writer should reject invalid model"
        );

        require(readBinaryFile(output) == std::vector<unsigned char>({ 'o', 'l', 'd' }), "failed write should not replace existing file");
        require(!std::filesystem::exists(output.string() + ".tmp"), "failed write should not leave temporary file");
    }

    void testDuplicateObjectResource()
    {
        const std::filesystem::path root =
            testRoot() / "duplicate_object";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=DuplicateObject\n"
            "path=game\n"
            "root=aaaa\n"
        );

        writeFile(
            root / "game" / "aaaa.json",
            "{ \"children\": { \"bbbb\": { \"shape\": { \"type\": \"block\" } } } }\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "project should compile before duplicate mutation");

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(CompiledProjectWriter::write(output.generic_string(), compiled.project, writeDiagnostics), "compiled project should write before duplicate mutation");

        std::vector<unsigned char> bytes =
            readBinaryFile(output);

        replaceAllAscii(
            bytes,
            "bbbb",
            "aaaa"
        );

        const std::filesystem::path broken =
            root / "duplicate.flxc";

        writeBinary(broken, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(broken.generic_string());

        require(!result.success, "duplicate object should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::DuplicateCompiledResource), "duplicate object should use stable code");
    }

    void testDuplicateScriptResource()
    {
        const std::filesystem::path root =
            testRoot() / "duplicate_script";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game" / "scripts");

        writeFile(
            root / "game.flx",
            "name=DuplicateScript\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{ \"behavior\": { \"scripts\": [\"scripts/aaaa\", \"scripts/bbbb\"] } }\n"
        );

        writeFile(
            root / "game" / "scripts" / "aaaa.js",
            "function action(object) {}\n"
        );

        writeFile(
            root / "game" / "scripts" / "bbbb.js",
            "function motion(object) {}\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "project should compile before duplicate script mutation");
        require(compiled.project.resources.scriptCount() == 2, "project should register two scripts");

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(CompiledProjectWriter::write(output.generic_string(), compiled.project, writeDiagnostics), "compiled project should write before duplicate script mutation");

        std::vector<unsigned char> bytes =
            readBinaryFile(output);

        replaceAllAscii(
            bytes,
            "bbbb",
            "aaaa"
        );

        const std::filesystem::path broken =
            root / "duplicate.flxc";

        writeBinary(broken, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(broken.generic_string());

        require(!result.success, "duplicate script should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::DuplicateCompiledResource), "duplicate script should use stable code");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "compiled project roundtrip minimal", testCompiledProjectRoundTripMinimal },

        { "compiled project roundtrip graph", testCompiledProjectRoundTripGraph },

        { "deterministic bytes", testDeterministicBytes },

        { "invalid compiled magic", testInvalidCompiledMagic },

        { "invalid compiled version", testInvalidCompiledVersion },

        { "truncated compiled project", testTruncatedCompiledProject },

        { "missing compiled file", testMissingFile },

        { "string limit exceeded", testStringLimitExceeded },

        { "collection limit exceeded", testCollectionLimitExceeded },

        { "invalid bool", testInvalidBool },

        { "trailing bytes", testTrailingBytes },

        { "missing root id fails read", testMissingRootIdFailsRead },

        { "broken childResources fail write", testBrokenChildResourcesFailWrite },

        { "embedded children fail write", testEmbeddedChildrenFailWrite },

        { "missing script fails write", testMissingScriptFailsWrite },

        { "no partial file on write failure", testNoPartialFileOnWriteFailure },

        { "duplicate object resource", testDuplicateObjectResource },

        { "duplicate script resource", testDuplicateScriptResource }

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

