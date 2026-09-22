#pragma once

#include "../../engine/compiler/ProjectCompiler.h"
#include "../../engine/cli/CliParser.h"
#include <filesystem>
#include <sstream>
#include <string>
#include <streambuf>
#include <vector>

namespace flx::test
{
    std::filesystem::path testRoot();

    void writeFile(
        const std::filesystem::path& path,
        const std::string& text
    );

    void writeBinary(
        const std::filesystem::path& path,
        const std::vector<unsigned char>& data
    );

    void require(bool condition, const std::string& message);

    std::size_t countOccurrences(
        const std::string& text,
        const std::string& needle
    );

    std::string readFirstLine(const std::filesystem::path& path);

    CompilationResult compile(const std::filesystem::path& path);

    CliParseResult parseArguments(const std::vector<std::string>& arguments);

    std::filesystem::path createMinimalProject(const std::string& name);

    class StreamCapture
    {
    public:
        StreamCapture();
        ~StreamCapture();

        std::ostringstream output;
        std::ostringstream error;

    private:
        std::streambuf* oldOutput = nullptr;
        std::streambuf* oldError = nullptr;
    };
}
