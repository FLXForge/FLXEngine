#include "TestSupport.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace flx::test
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

    std::size_t countOccurrences(
        const std::string& text,
        const std::string& needle
    )
    {
        if (needle.empty())
        {
            return 0;
        }

        std::size_t count = 0;
        std::size_t position = 0;

        while ((position = text.find(needle, position)) != std::string::npos)
        {
            ++count;
            position += needle.size();
        }

        return count;
    }

    std::string readFirstLine(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        std::string line;
        std::getline(file, line);
        return line;
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

    StreamCapture::StreamCapture()
        :
        oldOutput(std::cout.rdbuf(output.rdbuf())),
        oldError(std::cerr.rdbuf(error.rdbuf()))
    {
    }

    StreamCapture::~StreamCapture()
    {
        std::cout.rdbuf(oldOutput);
        std::cerr.rdbuf(oldError);
    }
}
