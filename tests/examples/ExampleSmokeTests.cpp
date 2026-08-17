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

        const std::vector<std::string> examples = {
            "pong.flx",
            "asteroids.flx",
            "arkanoid.flx",
            "invaders.flx",
            "scripting/input/input-demo.flx"
        };

        for (const std::string& example : examples)
        {
            CompilationResult compiled =
                compile(sourceRoot / "examples" / example);

            require(
                compiled.success,
                "known example should compile before compiled roundtrip: " + example
            );

            const std::filesystem::path output =
                testRoot() / "compiled_examples" /
                std::filesystem::path(example).replace_extension(".flxc");

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

    void testKnownExamples()
    {
        const std::filesystem::path sourceRoot =
            std::filesystem::path(FLX_SOURCE_DIR);

        const std::vector<std::string> examples = {
            "pong.flx",
            "asteroids.flx",
            "arkanoid.flx",
            "invaders.flx",
            "scripting/input/input-demo.flx"
        };

        for (const std::string& example : examples)
        {
            const CompilationResult result =
                compile(sourceRoot / "examples" / example);

            require(
                result.success,
                "known example should compile: " + example
            );

            require(
                result.project.resources.findObject(result.project.rootId) != nullptr,
                "known example should register root definition: " + example
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

