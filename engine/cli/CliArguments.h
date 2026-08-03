#pragma once

#include <filesystem>
#include <optional>

enum class CliCommand
{
    Run,
    Compile,
    RunCompiled,
    Validate,
    Version,
    Help
};

enum class CliOutputFormat
{
    Text,
    Json
};

struct CliArguments
{
    CliCommand command = CliCommand::Run;
    std::filesystem::path target = ".";
    std::optional<std::filesystem::path> output;
    std::optional<int> maxFrames;
    CliOutputFormat format = CliOutputFormat::Text;
    std::optional<CliCommand> helpCommand;
};
