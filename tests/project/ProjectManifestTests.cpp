#include "../support/TestSupport.h"
#include "../../engine/project/ProjectManifestLoader.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{
    ProjectManifestResult loadManifest(
        const std::string& name,
        const std::string& content
    )
    {
        const std::filesystem::path root =
            testRoot() / "project_manifest" / name;

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);

        const std::filesystem::path path =
            root / "project.flx";

        writeFile(path, content);

        return ProjectManifestLoader::load(path.generic_string());
    }

    void testMinimalManifest()
    {
        const ProjectManifestResult result =
            loadManifest(
                "minimal",
                "root=game\n"
            );

        require(result.success, "minimal manifest should load");
        require(result.manifest.root == "game", "root should load");
        require(result.manifest.path == ".", "path should default to dot");
    }

    void testMetadataAndFreeVersion()
    {
        const ProjectManifestResult result =
            loadManifest(
                "metadata",
                "name=Example\n"
                "version=prototype alpha\n"
                "notes=developer notes\n"
                "root=root\n"
            );

        require(result.success, "metadata manifest should load");
        require(result.manifest.metadata.name == "Example", "name should load");
        require(result.manifest.metadata.version == "prototype alpha", "version should be free text");
        require(result.manifest.metadata.notes == "developer notes", "notes should load");
    }

    void testTitleDefaultsToName()
    {
        const ProjectManifestResult result =
            loadManifest(
                "title_default",
                "name=Default Title\n"
                "root=root\n"
            );

        require(result.success, "title default manifest should load");
        require(result.manifest.title == "Default Title", "title should default to name");
    }

    void testMissingRootFails()
    {
        const ProjectManifestResult result =
            loadManifest(
                "missing_root",
                "name=No Root\n"
            );

        require(!result.success, "missing root should fail");
        require(result.diagnostics.hasErrors(), "missing root should report error");
    }

    void testEmptyRootFails()
    {
        const ProjectManifestResult result =
            loadManifest(
                "empty_root",
                "root=\n"
            );

        require(!result.success, "empty root should fail");
        require(result.diagnostics.hasErrors(), "empty root should report error");
    }

    void testInvalidLineFails()
    {
        const ProjectManifestResult result =
            loadManifest(
                "invalid_line",
                "root=root\n"
                "invalid line\n"
            );

        require(!result.success, "invalid line should fail");
        require(result.diagnostics.hasErrors(), "invalid line should report error");
    }

    void testUnknownFieldFails()
    {
        const ProjectManifestResult result =
            loadManifest(
                "unknown",
                "root=root\n"
                "unknown=value\n"
            );

        require(!result.success, "unknown field should fail");
        require(result.diagnostics.hasErrors(), "unknown field should report error");
    }

    void testDuplicateFieldFails()
    {
        const ProjectManifestResult result =
            loadManifest(
                "duplicate",
                "root=root\n"
                "root=other\n"
            );

        require(!result.success, "duplicate field should fail");
        require(result.diagnostics.hasErrors(), "duplicate field should report error");
    }

    void testRemovedFieldsFail()
    {
        const std::vector<std::string> fields = {
            "screen.width",
            "screen.height",
            "screen.scale",
            "window.mode",
            "debug.collisions",
            "debug.logs",
            "debug.console"
        };

        for (const std::string& field : fields)
        {
            const ProjectManifestResult result =
                loadManifest(
                    "removed_" + field,
                    "root=root\n" + field + "=value\n"
                );

            require(!result.success, "removed field should fail: " + field);
            require(result.diagnostics.hasErrors(), "removed field should report error: " + field);
        }
    }

    void testPathsArePreserved()
    {
        const ProjectManifestResult result =
            loadManifest(
                "paths",
                "path=../world\n"
                "root=/scenes/title\n"
                "machine=C:/machines/std.yml\n"
                "input.mapping=input/default.input\n"
            );

        require(result.success, "paths manifest should load");
        require(result.manifest.path == "../world", "path should be preserved");
        require(result.manifest.root == "/scenes/title", "root should be preserved");
        require(result.manifest.machine == "C:/machines/std.yml", "machine should be preserved");
        require(result.manifest.inputMapping == "input/default.input", "input mapping should be preserved");
    }
}

int main()
{
    const std::vector<std::pair<std::string, void(*)()>> tests = {
        { "minimal manifest", testMinimalManifest },
        { "metadata and free version", testMetadataAndFreeVersion },
        { "title defaults to name", testTitleDefaultsToName },
        { "missing root fails", testMissingRootFails },
        { "empty root fails", testEmptyRootFails },
        { "invalid line fails", testInvalidLineFails },
        { "unknown field fails", testUnknownFieldFails },
        { "duplicate field fails", testDuplicateFieldFails },
        { "removed fields fail", testRemovedFieldsFail },
        { "paths are preserved", testPathsArePreserved }
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
