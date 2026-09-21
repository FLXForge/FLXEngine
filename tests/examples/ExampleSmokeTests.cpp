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

    void testCompiledExamplesRoundTrip()
    {
        const std::filesystem::path sourceRoot =
            std::filesystem::path(FLX_SOURCE_DIR);

        const std::vector<std::filesystem::path> examples = {
            sourceRoot / "examples" / "pong.flx",
            sourceRoot / "examples" / "asteroids.flx",
            sourceRoot / "examples" / "arkanoid.flx",
            sourceRoot / "examples" / "invaders.flx",
            sourceRoot / "microexamples" / "input" / "digital" / "input-demo.flx"
        };

        for (const std::filesystem::path& example : examples)
        {
            CompilationResult compiled =
                compile(example);

            require(
                compiled.success,
                "known example should compile before compiled roundtrip: " + example.generic_string()
            );

            const std::filesystem::path output =
                testRoot() / "compiled_examples" /
                example.filename().replace_extension(".flxc");

            Diagnostics writeDiagnostics;

            require(
                CompiledProjectWriter::write(
                    output.generic_string(),
                    compiled.project,
                    writeDiagnostics
                ),
                "known example should write compiled file: " + example.generic_string()
            );

            CompiledProjectBinaryResult loaded =
                CompiledProjectReader::read(output.generic_string());

            require(
                loaded.success,
                "known example should read compiled file: " + example.generic_string()
            );

            require(
                loaded.project.resources.objectCount() == compiled.project.resources.objectCount(),
                "known example object count should survive compiled roundtrip: " + example.generic_string()
            );
        }
    }

    void testKnownExamples()
    {
        const std::filesystem::path sourceRoot =
            std::filesystem::path(FLX_SOURCE_DIR);

        const std::vector<std::filesystem::path> examples = {
            sourceRoot / "examples" / "pong.flx",
            sourceRoot / "examples" / "asteroids.flx",
            sourceRoot / "examples" / "arkanoid.flx",
            sourceRoot / "examples" / "invaders.flx",
            sourceRoot / "microexamples" / "input" / "digital" / "input-demo.flx"
        };

        for (const std::filesystem::path& example : examples)
        {
            const CompilationResult result =
                compile(example);

            require(
                result.success,
                "known example should compile: " + example.generic_string()
            );

            require(
                result.project.resources.findObject(result.project.rootId) != nullptr,
                "known example should register root definition: " + example.generic_string()
            );
        }
    }

}

int main()

{

    const std::vector<std::pair<std::string, void(*)()>> tests = {

        { "compiled examples roundtrip", testCompiledExamplesRoundTrip },

        { "known examples", testKnownExamples }

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

