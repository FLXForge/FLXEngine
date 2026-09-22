#pragma once

#include "../core/RunOptions.h"

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
    RunOptions runOptions;
    CliOutputFormat format = CliOutputFormat::Text;
    std::optional<CliCommand> helpCommand;
};
