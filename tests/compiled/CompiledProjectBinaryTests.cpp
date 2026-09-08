#include "../support/TestSupport.h"
#include "../../engine/compiler/CompiledProjectBinary.h"
#include "../../engine/compiler/binary/BinaryLimits.h"
#include "../../engine/compiler/binary/CompiledProjectCodec.h"
#include "../../engine/runtime/RuntimeWorld.h"
#include "../../engine/scripting/ScriptEngine.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
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

    const ScriptValue& localValue(
        const ObjectDefinition& object,
        const std::string& key
    )
    {
        const auto it =
            object.local.find(key);

        require(it != object.local.end(), "local key should exist");
        return it->second;
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

    void replaceNthAscii(
        std::vector<unsigned char>& bytes,
        const std::string& from,
        const std::string& to,
        int occurrence
    )
    {
        require(from.size() == to.size(), "replacement must keep binary size");
        require(occurrence > 0, "occurrence must be positive");

        auto searchFrom =
            bytes.begin();

        for (int i = 1; i <= occurrence; ++i)
        {
            auto it =
                std::search(
                    searchFrom,
                    bytes.end(),
                    from.begin(),
                    from.end()
                );

            require(it != bytes.end(), "binary text occurrence should exist");

            if (i == occurrence)
            {
                std::copy(
                    to.begin(),
                    to.end(),
                    it
                );

                return;
            }

            searchFrom =
                it + static_cast<std::ptrdiff_t>(from.size());
        }
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

    void appendF32(
        std::vector<unsigned char>& bytes,
        float value
    )
    {
        static_assert(sizeof(float) == 4);

        uint32_t raw = 0;
        std::memcpy(&raw, &value, sizeof(float));
        appendU32(bytes, raw);
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
        appendU32(bytes, flx::binary::CompiledProjectCodec::FormatVersion);
        appendString(bytes, "test-producer");

        for (int i = 0; i < 7; ++i)
        {
            appendString(bytes, "");
        }

        appendU32(bytes, 0u);
        appendU32(bytes, 0u);

        appendI32(bytes, 640);
        appendI32(bytes, 480);
        appendString(bytes, "black");
        appendU8(bytes, 0);
        appendU32(bytes, 0);
        appendI32(bytes, 0);
        appendI32(bytes, 0);
        appendI32(bytes, 0);
        appendU8(bytes, 0);
        appendString(bytes, "");
        appendI32(bytes, 0);
        appendU8(bytes, 0);
        appendU8(bytes, 1);
        appendU8(bytes, 0);
        appendU8(bytes, 0);
        appendI32(bytes, 1);
        appendU8(bytes, 0);

        appendI32(bytes, 8);
        appendI32(bytes, 16);
        appendString(bytes, "shared");
        appendString(bytes, "replace_oldest");
        appendString(bytes, "open");
        appendString(bytes, "rich");
        appendString(bytes, "expressive");
        appendString(bytes, "rich");
        appendString(bytes, "high");
        appendString(bytes, "expressive");
        appendString(bytes, "stereo");
        appendU8(bytes, 1);
        appendU8(bytes, 1);
        appendU8(bytes, 1);
        appendString(bytes, "all");

        appendI32(bytes, 16);
        appendI32(bytes, 16);
        appendU32(bytes, 1);
        appendString(bytes, "4way");
        appendString(bytes, "last");
        appendF32(bytes, 0.0f);
        appendI32(bytes, 16);
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

    CompiledProject makeSingleObjectProject()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";

        require(
            project.resources.addObject(
                project.rootId,
                root
            ),
            "root should register"
        );

        return project;
    }

    std::vector<unsigned char> writeProjectBytes(
        const std::string& name,
        const CompiledProject& project
    )
    {
        const std::filesystem::path output =
            testRoot() / name / "game.flxc";

        Diagnostics diagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                project,
                diagnostics
            ),
            "compiled project should write for byte mutation"
        );

        return readBinaryFile(output);
    }

    CompiledProjectBinaryResult readMutatedBytes(
        const std::string& name,
        const std::vector<unsigned char>& bytes
    )
    {
        const std::filesystem::path path =
            testRoot() / name / "broken.flxc";

        writeBinary(
            path,
            bytes
        );

        return CompiledProjectReader::read(path.generic_string());
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
        require(loaded.metadata.formatVersion == 8, "format version should be exposed");
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

    void testCompiledProjectRoundTripInputV7()
    {
        const std::filesystem::path root =
            testRoot() / "roundtrip_input_v7";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");
        std::filesystem::create_directories(root / "machines");

        writeFile(
            root / "game.flx",
            "name=RoundTripInput\n"
            "path=game\n"
            "root=root\n"
            "machine=machines/input.machine.yml\n"
        );

        writeFile(
            root / "machines" / "input.machine.yml",
            "machine:\n"
            "  input:\n"
            "    system:\n"
            "      buttons: 9\n"
            "    players:\n"
            "      count: 3\n"
            "      controls:\n"
            "        directions:\n"
            "          - type: 2way\n"
            "            simultaneous: first\n"
            "            buffer: 0.25\n"
            "          - type: 4way\n"
            "            simultaneous: neutral\n"
            "            buffer: 0\n"
            "        buttons: 12\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{\n"
            "  \"control\": { \"player\": 2 },\n"
            "  \"local\": {\n"
            "    \"score\": 12,\n"
            "    \"ready\": true,\n"
            "    \"label\": \"start\"\n"
            "  }\n"
            "}\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "input project should compile");

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "input project should write"
        );

        CompiledProjectBinaryResult loaded =
            CompiledProjectReader::read(output.generic_string());

        require(loaded.success, "input project should read");
        require(loaded.metadata.formatVersion == 8, "input roundtrip should use current format");
        require(loaded.project.context.machine.input.systemButtons == 9, "system button count should survive roundtrip");
        require(loaded.project.context.machine.input.players == 3, "player count should survive roundtrip");
        require(loaded.project.context.machine.input.playerButtons == 12, "player button count should survive roundtrip");
        require(loaded.project.context.machine.input.directions.size() == 2, "direction collection should survive roundtrip");
        require(loaded.project.context.machine.input.directions[0].type == "2way", "direction type should survive roundtrip");
        require(loaded.project.context.machine.input.directions[0].simultaneous == "first", "direction simultaneous policy should survive roundtrip");
        require(loaded.project.context.machine.input.directions[0].buffer == 0.25f, "direction buffer should survive roundtrip");
        require(loaded.project.context.machine.input.directions[1].type == "4way", "second direction type should survive roundtrip");
        require(loaded.project.context.machine.input.directions[1].simultaneous == "neutral", "second direction simultaneous policy should survive roundtrip");
        require(loaded.project.context.inputMapping.players.count(1) == 1, "compiled input mapping player should survive roundtrip");
        require(loaded.project.context.inputMapping.players.at(1).directions.size() == 1, "compiled input mapping should preserve mapped directions without trimming to chip");
        require(loaded.project.context.inputMapping.players.at(1).directions[0].components.count(InputComponent::Positive) == 1, "2way default up/right should be normalized to positive in compiled mapping");
        require(loaded.project.context.inputMapping.players.at(1).directions[0].components.count(InputComponent::Negative) == 1, "2way default down/left should be normalized to negative in compiled mapping");
        require(loaded.project.context.inputMapping.systemButtons.count(0) == 1, "compiled system mapping should survive roundtrip");
        require(rootObject(loaded.project).controlPlayer == 2, "control.player should survive roundtrip");
        require(std::get<double>(localValue(rootObject(loaded.project), "score")) == 12.0, "numeric local value should survive roundtrip");
        require(std::get<bool>(localValue(rootObject(loaded.project), "ready")), "boolean local value should survive roundtrip");
        require(std::get<std::string>(localValue(rootObject(loaded.project), "label")) == "start", "string local value should survive roundtrip");
    }

    void testCompiledProjectRoundTripDefaultMachineInputV7()
    {
        const std::filesystem::path root =
            testRoot() / "roundtrip_default_input_v7";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "game");

        writeFile(
            root / "game.flx",
            "name=RoundTripDefaultInput\n"
            "path=game\n"
            "root=root\n"
        );

        writeFile(
            root / "game" / "root.json",
            "{}\n"
        );

        CompilationResult compiled =
            compile(root / "game.flx");

        require(compiled.success, "default input project should compile");
        require(compiled.project.context.machine.input.directions.size() == 1, "compiled default machine should expose one direction");
        require(compiled.project.context.machine.input.directions[0].type == "4way", "compiled default direction should be 4way");
        require(compiled.project.context.machine.input.directions[0].simultaneous == "last", "compiled default direction policy should be last");
        require(compiled.project.context.machine.input.directions[0].buffer == 0.0f, "compiled default direction buffer should be zero");

        const std::filesystem::path output =
            root / "game.flxc";

        Diagnostics writeDiagnostics;

        require(
            CompiledProjectWriter::write(
                output.generic_string(),
                compiled.project,
                writeDiagnostics
            ),
            "default input project should write"
        );

        CompiledProjectBinaryResult loaded =
            CompiledProjectReader::read(output.generic_string());

        require(loaded.success, "default input project should read");
        require(loaded.metadata.formatVersion == 8, "default input roundtrip should use current format");
        require(loaded.project.context.machine.input.systemButtons == 16, "default system button count should survive roundtrip");
        require(loaded.project.context.machine.input.players == 16, "default player count should survive roundtrip");
        require(loaded.project.context.machine.input.playerButtons == 16, "default player button count should survive roundtrip");
        require(loaded.project.context.machine.input.directions.size() == 1, "default direction collection should survive roundtrip");
        require(loaded.project.context.machine.input.directions[0].type == "4way", "default direction type should survive roundtrip");
        require(loaded.project.context.machine.input.directions[0].simultaneous == "last", "default direction policy should survive roundtrip");
        require(loaded.project.context.machine.input.directions[0].buffer == 0.0f, "default direction buffer should survive roundtrip");
        require(loaded.project.context.inputMapping.players.count(1) == 1, "default compiled input mapping should survive roundtrip");
        require(loaded.project.context.inputMapping.players.at(1).directions[0].components.count(InputComponent::Up) == 1, "default up mapping should survive roundtrip");
        require(loaded.project.context.inputMapping.players.at(1).buttons.count(2) == 1, "default player button 2 mapping should survive roundtrip");
        require(loaded.project.context.inputMapping.players.at(1).buttons.count(3) == 1, "default player button 3 mapping should survive roundtrip");
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
        appendU32(bytes, flx::binary::CompiledProjectCodec::FormatVersion);
        appendU32(bytes, flx::binary::MaxStringSize + 1u);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "string limit should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "string limit should use limit diagnostic");
    }

    void testTotalStringBudgetExceeded()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "total_string_budget.flxc";

        std::vector<unsigned char> bytes;
        appendU32(bytes, 0x43584C46u);
        appendU32(bytes, flx::binary::CompiledProjectCodec::FormatVersion);
        appendU32(bytes, flx::binary::MaxTotalDecodedStringBytes + 1u);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "total string budget should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "total string budget should use limit diagnostic");
    }

    void testCollectionLimitExceeded()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "collection_limit.flxc";

        std::vector<unsigned char> bytes;
        appendMinimalHeaderAndContext(bytes);
        appendString(bytes, "");
        appendU32(bytes, flx::binary::MaxCollectionCount + 1u);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "collection limit should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "collection limit should use limit diagnostic");
    }

    void testTotalElementBudgetExceeded()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "total_element_budget.flxc";

        std::vector<unsigned char> bytes;
        appendMinimalHeaderAndContext(bytes);
        appendString(bytes, "");
        appendU32(bytes, flx::binary::MaxTotalDecodedElements + 1u);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "total element budget should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "total element budget should use limit diagnostic");
    }

    void testPhysicalFileSizeLimitExceeded()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "physical_size_limit.flxc";

        std::filesystem::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        require(file.is_open(), "large placeholder file should open");
        file.seekp(static_cast<std::streamoff>(flx::binary::MaxBinaryFileSize));
        file.put('\0');
        file.close();

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "physical file size limit should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "physical size limit should use limit diagnostic");
    }

    void testWriterPhysicalFileSizeLimitKeepsExistingOutput()
    {
        CompiledProject project =
            makeSingleObjectProject();

        project.context.name =
            std::string(flx::binary::MaxStringSize, 'a');

        project.context.version =
            std::string(flx::binary::MaxStringSize, 'b');

        const std::filesystem::path output =
            testRoot() / "writer_physical_size_limit" / "game.flxc";

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
            "writer should reject temporary file above physical size limit"
        );

        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompiledProjectLimitExceeded), "writer physical size limit should use limit diagnostic");
        require(readBinaryFile(output) == std::vector<unsigned char>({ 'o', 'l', 'd' }), "physical size failure should keep existing output intact");
        require(!std::filesystem::exists(output.string() + ".tmp"), "physical size failure should remove temporary file");
    }

    void testInvalidBool()
    {
        const std::filesystem::path path =
            testRoot() / "invalid" / "invalid_bool.flxc";

        std::vector<unsigned char> bytes;
        appendU32(bytes, 0x43584C46u);
        appendU32(bytes, flx::binary::CompiledProjectCodec::FormatVersion);
        appendString(bytes, "test-producer");

        for (int i = 0; i < 7; ++i)
        {
            appendString(bytes, "");
        }

        appendU32(bytes, 0u);
        appendU32(bytes, 0u);

        appendI32(bytes, 640);
        appendI32(bytes, 480);
        appendString(bytes, "black");
        appendU8(bytes, 2);

        writeBinary(path, bytes);

        CompiledProjectBinaryResult result =
            CompiledProjectReader::read(path.generic_string());

        require(!result.success, "invalid bool should fail");
        require(result.diagnostics.hasErrors(), "invalid bool should report diagnostics");
        require(
            hasDiagnosticCode(result.diagnostics, DiagnosticCode::InvalidCompiledProjectValue),
            "invalid bool should use invalid value diagnostic, got " +
            result.diagnostics.all().front().identifier +
            " at " +
            result.diagnostics.all().front().field
        );
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
        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompiledProjectMissingChildResource), "model invariant should use missing child diagnostic");
    }

    void testEmbeddedChildrenFailWrite()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.children["child"] =
            std::make_shared<ObjectDefinition>();
        root.children["child"]->id = "child";

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
        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompiledProjectEmbeddedChildren), "embedded children should use stable diagnostic");
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
        require(hasDiagnosticCode(diagnostics, DiagnosticCode::CompiledProjectMissingScriptResource), "missing script should use stable diagnostic");
    }

    void testNoPartialFileOnWriteFailure()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.children["child"] =
            std::make_shared<ObjectDefinition>();
        root.children["child"]->id = "child";

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

    void testDuplicateMusicEntry()
    {
        CompiledProject project =
            makeSingleObjectProject();

        ObjectDefinition root =
            *project.resources.findObject("root");

        MusicDefinition first;
        MusicDefinition second;
        first.tempo = 120.0f;
        second.tempo = 90.0f;
        root.music["aaaa"] = first;
        root.music["bbbb"] = second;

        project = CompiledProject();
        project.rootId = "root";
        require(project.resources.addObject("root", root), "root with music should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("duplicate_music_entry", project);

        replaceFirstAscii(
            bytes,
            "bbbb",
            "aaaa"
        );

        CompiledProjectBinaryResult result =
            readMutatedBytes("duplicate_music_entry", bytes);

        require(!result.success, "duplicate music entry should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::DuplicateCompiledEntry), "duplicate music should use entry diagnostic");
    }

    void testDuplicateSoundEntry()
    {
        CompiledProject project =
            makeSingleObjectProject();

        ObjectDefinition root =
            *project.resources.findObject("root");

        SoundDefinition first;
        SoundDefinition second;
        first.duration = 0.1f;
        second.duration = 0.2f;
        root.sounds["aaaa"] = first;
        root.sounds["bbbb"] = second;

        project = CompiledProject();
        project.rootId = "root";
        require(project.resources.addObject("root", root), "root with sounds should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("duplicate_sound_entry", project);

        replaceFirstAscii(
            bytes,
            "bbbb",
            "aaaa"
        );

        CompiledProjectBinaryResult result =
            readMutatedBytes("duplicate_sound_entry", bytes);

        require(!result.success, "duplicate sound entry should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::DuplicateCompiledEntry), "duplicate sound should use entry diagnostic");
    }

    void testDuplicateChildResourceEntry()
    {
        CompiledProject project;
        project.rootId = "root";

        ObjectDefinition root;
        root.id = "root";
        root.childResources["aaaa"] = "child_one";
        root.childResources["bbbb"] = "child_two";

        ObjectDefinition childOne;
        childOne.id = "child_one";

        ObjectDefinition childTwo;
        childTwo.id = "child_two";

        require(project.resources.addObject("root", root), "root should register");
        require(project.resources.addObject("child_one", childOne), "first child should register");
        require(project.resources.addObject("child_two", childTwo), "second child should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("duplicate_child_entry", project);

        replaceFirstAscii(
            bytes,
            "bbbb",
            "aaaa"
        );

        CompiledProjectBinaryResult result =
            readMutatedBytes("duplicate_child_entry", bytes);

        require(!result.success, "duplicate child resource entry should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::DuplicateCompiledEntry), "duplicate child resource should use entry diagnostic");
    }

    void testDuplicateStateEntry()
    {
        CompiledProject project =
            makeSingleObjectProject();

        ObjectDefinition root =
            *project.resources.findObject("root");

        root.initialState = "aaaa";
        root.stateTransitions["aaaa"] = { "bbbb" };
        root.stateTransitions["bbbb"] = { "aaaa" };

        project = CompiledProject();
        project.rootId = "root";
        require(project.resources.addObject("root", root), "root with states should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("duplicate_state_entry", project);

        replaceNthAscii(
            bytes,
            "bbbb",
            "aaaa",
            2
        );

        CompiledProjectBinaryResult result =
            readMutatedBytes("duplicate_state_entry", bytes);

        require(!result.success, "duplicate state entry should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::DuplicateCompiledEntry), "duplicate state should use entry diagnostic");
    }

    void testObjectKeyIdMismatch()
    {
        CompiledProject project;
        project.rootId = "root#obj";

        ObjectDefinition root;
        root.id = "root#obj";

        require(project.resources.addObject("root#obj", root), "compiled-id root should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("object_key_id_mismatch", project);

        replaceNthAscii(
            bytes,
            "root#obj",
            "xxxx#obj",
            3
        );

        CompiledProjectBinaryResult result =
            readMutatedBytes("object_key_id_mismatch", bytes);

        require(!result.success, "object key/id mismatch should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectIdentityMismatch), "object key/id mismatch should use stable diagnostic");
    }

    void testScriptKeyIdMismatch()
    {
        CompiledProject project =
            makeSingleObjectProject();

        ObjectDefinition root =
            *project.resources.findObject("root");
        root.resolvedScriptPaths.push_back("aaaa");

        project = CompiledProject();
        project.rootId = "root";
        require(project.resources.addObject("root", root), "root with script should register");

        ScriptResource script;
        script.id = "aaaa";
        script.sourceName = "script.js";
        script.code = "function action(object) {}";
        require(project.resources.addScript("aaaa", script), "script should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("script_key_id_mismatch", project);

        replaceNthAscii(
            bytes,
            "aaaa",
            "bbbb",
            3
        );

        CompiledProjectBinaryResult result =
            readMutatedBytes("script_key_id_mismatch", bytes);

        require(!result.success, "script key/id mismatch should fail");
        require(hasDiagnosticCode(result.diagnostics, DiagnosticCode::CompiledProjectIdentityMismatch), "script key/id mismatch should use stable diagnostic");
    }

    void testWriterReaderAndRuntimeUseSameInvariantDiagnostic()
    {
        CompiledProject project;
        project.rootId = "root#obj";

        ObjectDefinition root;
        root.id = "xxxx#obj";

        require(project.resources.addObject("root#obj", root), "mismatched root should register for invariant test");

        Diagnostics writeDiagnostics;

        require(
            !CompiledProjectWriter::write(
                (testRoot() / "shared_invariant" / "writer.flxc").generic_string(),
                project,
                writeDiagnostics
            ),
            "writer should reject identity mismatch"
        );

        require(hasDiagnosticCode(writeDiagnostics, DiagnosticCode::CompiledProjectIdentityMismatch), "writer should use identity mismatch diagnostic");

        CompiledProject validProject;
        validProject.rootId = "root#obj";

        ObjectDefinition validRoot;
        validRoot.id = "root#obj";

        require(validProject.resources.addObject("root#obj", validRoot), "valid compiled-id root should register");

        std::vector<unsigned char> bytes =
            writeProjectBytes("shared_invariant", validProject);

        replaceNthAscii(
            bytes,
            "root#obj",
            "xxxx#obj",
            3
        );

        CompiledProjectBinaryResult readResult =
            readMutatedBytes("shared_invariant", bytes);

        require(!readResult.success, "reader should reject identity mismatch");
        require(hasDiagnosticCode(readResult.diagnostics, DiagnosticCode::CompiledProjectIdentityMismatch), "reader should use identity mismatch diagnostic");

        RuntimeWorld world;
        ScriptEngine scriptEngine;
        RuntimeLoadResult loadResult =
            world.load(
            project,
            scriptEngine
        );

        require(hasDiagnosticCode(loadResult.diagnostics, DiagnosticCode::CompiledProjectIdentityMismatch), "runtime should use identity mismatch diagnostic");
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "compiled project roundtrip minimal", testCompiledProjectRoundTripMinimal },

        { "compiled project roundtrip graph", testCompiledProjectRoundTripGraph },

        { "compiled project roundtrip input current format", testCompiledProjectRoundTripInputV7 },

        { "compiled project roundtrip default machine input current format", testCompiledProjectRoundTripDefaultMachineInputV7 },

        { "deterministic bytes", testDeterministicBytes },

        { "invalid compiled magic", testInvalidCompiledMagic },

        { "invalid compiled version", testInvalidCompiledVersion },

        { "truncated compiled project", testTruncatedCompiledProject },

        { "missing compiled file", testMissingFile },

        { "string limit exceeded", testStringLimitExceeded },

        { "total string budget exceeded", testTotalStringBudgetExceeded },

        { "collection limit exceeded", testCollectionLimitExceeded },

        { "total element budget exceeded", testTotalElementBudgetExceeded },

        { "physical file size limit exceeded", testPhysicalFileSizeLimitExceeded },

        { "writer physical file size limit keeps existing output", testWriterPhysicalFileSizeLimitKeepsExistingOutput },

        { "invalid bool", testInvalidBool },

        { "trailing bytes", testTrailingBytes },

        { "missing root id fails read", testMissingRootIdFailsRead },

        { "broken childResources fail write", testBrokenChildResourcesFailWrite },

        { "embedded children fail write", testEmbeddedChildrenFailWrite },

        { "missing script fails write", testMissingScriptFailsWrite },

        { "no partial file on write failure", testNoPartialFileOnWriteFailure },

        { "duplicate object resource", testDuplicateObjectResource },

        { "duplicate script resource", testDuplicateScriptResource },

        { "duplicate music entry", testDuplicateMusicEntry },

        { "duplicate sound entry", testDuplicateSoundEntry },

        { "duplicate child resource entry", testDuplicateChildResourceEntry },

        { "duplicate state entry", testDuplicateStateEntry },

        { "object key id mismatch", testObjectKeyIdMismatch },

        { "script key id mismatch", testScriptKeyIdMismatch },

        { "writer reader and runtime use same invariant diagnostic", testWriterReaderAndRuntimeUseSameInvariantDiagnostic }

    };

    for (const auto& test : tests)

    {

        try

        {

            std::cout << "[RUN] " << test.first << std::endl;

            test.second();

            std::cout << "[PASS] " << test.first << std::endl;

        }

        catch (const std::exception& exception)

        {

            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << "\n";

            return 1;

        }

    }

    return 0;

}

