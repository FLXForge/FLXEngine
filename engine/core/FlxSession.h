#pragma once

#include "../compiler/CompiledProject.h"
#include <memory>
#include <raylib.h>
#ifdef FLX_TESTING
#include "../runtime/ScriptValue.h"
#include <optional>
class RuntimeObject;
class AudioSystem;
#endif

class PhysicalInputProvider;

enum class FlxSessionState { Empty, Ready, Running, Finished, Failed };
enum class FlxSessionExitReason { None, ScriptRequestedExit, HostRequestedStop, LoadFailed, RuntimeFailed };

struct FlxSessionResult
{
    bool success = false;
    FlxSessionExitReason exitReason = FlxSessionExitReason::None;
    Diagnostics diagnostics;
    int framesExecuted = 0;
};

struct FlxSessionOptions
{
    // Disabled services allow deterministic logical execution without devices.
    bool enableVideo = true;
    bool enableAudio = true;
    bool debugCollisions = false;
    // Borrowed: must outlive the loaded session. nullptr selects raylib input.
    PhysicalInputProvider* inputProvider = nullptr;
};

// Single-threaded, non-reentrant host API. Owns execution, never physical devices.
class FlxSession
{
public:
    FlxSession();
    ~FlxSession();
    FlxSession(const FlxSession&) = delete;
    FlxSession& operator=(const FlxSession&) = delete;

    bool load(const CompiledProject& project, const FlxSessionOptions& options = {});
    bool update(float deltaTime);
    bool render();
    void requestStop();
    void unload();
    FlxSessionState state() const;
    bool finished() const;
    const FlxSessionResult& result() const;
    // Borrowed texture; never unload it. Invalidated by unload or failed execution.
    Texture2D output() const;

#ifdef FLX_TESTING
    std::optional<ScriptValue> testGlobalValue(const std::string& name) const;
    RuntimeObject* testObject(const std::string& name);
    AudioSystem* testAudioSystem();
#endif

private:
    struct Impl;
    void finish(FlxSessionExitReason reason);
    bool fail(DiagnosticCode code, const std::string& message, FlxSessionExitReason reason);
    std::unique_ptr<Impl> impl;
    FlxSessionState currentState = FlxSessionState::Empty;
    FlxSessionResult lastResult;
};
