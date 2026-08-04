#include "../support/TestSupport.h"
#include "../../engine/cli/ProjectResolver.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace flx::test;

namespace
{

    void testCliProjectResolver()
    {
        const std::filesystem::path root =
            testRoot() / "cli_resolver";

        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);

        writeFile(root / "direct.flx", "name=Direct\npath=game\nroot=root\n");
        writeFile(root / "notes.txt", "not a project\n");

        ProjectResolver resolver;
        ProjectResolutionResult result =
            resolver.resolve(root / "direct.flx");

        require(result.success, "direct .flx should resolve");
        require(result.manifestPath.filename() == "direct.flx", "direct .flx should be returned");

        result =
            resolver.resolve(root / "notes.txt");

        require(!result.success, "direct non .flx file should fail");

        writeFile(root / "upper.FLX", "name=Upper\npath=game\nroot=root\n");

        result =
            resolver.resolve(root / "upper.FLX");

        require(result.success, "direct .FLX should resolve");
        require(result.manifestPath.filename() == "upper.FLX", "direct .FLX should be returned");

        const std::filesystem::path projectDir =
            root / "project_dir";

        std::filesystem::create_directories(projectDir);
        writeFile(projectDir / "project.flx", "name=Project\npath=game\nroot=root\n");
        writeFile(projectDir / "other.flx", "name=Other\npath=game\nroot=root\n");

        result =
            resolver.resolve(projectDir);

        require(result.success, "directory with project.flx should resolve");
        require(result.manifestPath.filename() == "project.flx", "project.flx should be preferred over other .flx files");

        const std::filesystem::path singleDir =
            root / "single_dir";

        std::filesystem::create_directories(singleDir);
        writeFile(singleDir / "single.flx", "name=Single\npath=game\nroot=root\n");

        result =
            resolver.resolve(singleDir);

        require(result.success, "directory with one .flx should resolve");
        require(result.manifestPath.filename() == "single.flx", "single .flx should be selected");

        const std::filesystem::path upperSingleDir =
            root / "upper_single_dir";

        std::filesystem::create_directories(upperSingleDir);
        writeFile(upperSingleDir / "single.FLX", "name=UpperSingle\npath=game\nroot=root\n");

        result =
            resolver.resolve(upperSingleDir);

        require(result.success, "directory with one .FLX should resolve");
        require(result.manifestPath.filename() == "single.FLX", "single .FLX should be selected");

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

