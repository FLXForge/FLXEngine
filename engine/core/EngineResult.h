#pragma once

#include "../diagnostics/Diagnostics.h"

enum class EngineExitReason
{
    WindowClosed,
    ScriptRequestedExit,
    FrameLimitReached,
    InitializationFailed,
    RuntimeLoadFailed
};

struct EngineResult
{
    bool success = false;
    EngineExitReason exitReason = EngineExitReason::InitializationFailed;
    Diagnostics diagnostics;
    int framesExecuted = 0;
};
