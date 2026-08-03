#pragma once

enum class CliExitCode : int
{
    Success = 0,
    InternalError = 1,
    InvalidArguments = 2,
    ProjectResolutionError = 3,
    CompilationError = 4,
    InvalidCompiledProject = 5,
    RuntimeInitializationError = 6,
    RuntimeExecutionError = 7,
    BuildError = 8
};
