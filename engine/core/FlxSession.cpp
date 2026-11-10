#include "FlxSession.h"
#include "../audio/AudioSystem.h"
#include "../graphics/FadeSystem.h"
#include "../input/InputSystem.h"
#include "../runtime/RuntimeWorld.h"
#include "../scripting/ScriptEngine.h"
#include "../machine/VideoColorProcessor.h"
#include "../tools/ColorParser.h"
#include <cmath>
#include <exception>
#include <stdexcept>

struct FlxSession::Impl
{
    explicit Impl(const CompiledProject& source, const FlxSessionOptions& settings)
        : project(source), options(settings), context(project.context) {}

    ~Impl()
    {
        // Callbacks and JS views go first, while their world and project still exist.
        scriptEngine.reset();
        world.reset();
        audioSystem.shutdown();
        if (renderTarget.id != 0) UnloadRenderTexture(renderTarget);
    }
    void configureScriptEngine();
    CompiledProject project;
    FlxSessionOptions options;
    FlxContext& context;
    InputSystem inputSystem;
    FadeSystem fadeSystem;
    AudioSystem audioSystem;
    RenderTexture2D renderTarget{};
    Color backgroundColor = BLACK;
    std::unique_ptr<RuntimeWorld> world;
    std::unique_ptr<ScriptEngine> scriptEngine;
};

void FlxSession::Impl::configureScriptEngine()
{
    scriptEngine->setVideoChip(&context.machine.video);
    scriptEngine->setInputSystem(&inputSystem);
    scriptEngine->setFadeSystem(&fadeSystem);
    scriptEngine->setAudioSystem(&audioSystem);

    scriptEngine->setFindObjectFunction(
        [this](const std::string& name)
        {
            return world->findByName(name);
        }
    );

    scriptEngine->setFindObjectsByNameFunction(
        [this](const std::string& name)
        {
            return world->findAllLiveByName(name);
        }
    );

    scriptEngine->setFindObjectByIdFunction(
        [this](const std::string& id)
        {
            return world->findByRuntimeId(id);
        }
    );

    scriptEngine->setFindParentFunction(
        [this](const std::string& runtimeId)
        {
            return world->findLiveParent(runtimeId);
        }
    );

    scriptEngine->setFindChildrenFunction(
        [this](const std::string& runtimeId)
        {
            return world->findLiveChildren(runtimeId);
        }
    );

    scriptEngine->setSpawnObjectFunction(
        [this](
            RuntimeObject& source,
            const std::string& resourceId
            )
        {
            world->spawn(
                source,
                resourceId,
                *scriptEngine
            );
        }
    );

    scriptEngine->setCreationActiveFunction(
        [this](const RuntimeObject& object)
        {
            return world->creationActive(object);
        }
    );

    scriptEngine->setRayCastFunction(
        [this](
            RuntimeObject& source,
            ScriptEngine& activeScriptEngine,
            float angle,
            float distance
            )
        {
            return world->rayCast(
                source,
                activeScriptEngine,
                angle,
                distance
            );
        }
    );

    scriptEngine->setKeepOnlyFunction(
        [this](const std::string& runtimeId)
        {
            world->keepOnly(runtimeId);
        }
    );

    scriptEngine->setKillObjectFunction(
        [this](const std::string& runtimeId)
        {
            world->kill(runtimeId);
        }
    );

    scriptEngine->setShowObjectFunction(
        [this](const std::string& runtimeId)
        {
            world->show(runtimeId);
        }
    );

    scriptEngine->setHideObjectFunction(
        [this](const std::string& runtimeId)
        {
            world->hide(runtimeId);
        }
    );
}

FlxSession::FlxSession() = default;
FlxSession::~FlxSession() = default;

bool FlxSession::markFailed(FlxSessionExitReason reason)
{
    impl.reset();
    currentState = FlxSessionState::Failed;
    lastResult.success = false;
    lastResult.exitReason = reason;
    return false;
}

bool FlxSession::fail(DiagnosticCode code, const std::string& message, FlxSessionExitReason reason,
    const char* stage)
{
    lastResult.diagnostics.error(code, message, "session", stage);
    return markFailed(reason);
}

bool FlxSession::load(const CompiledProject& project, const FlxSessionOptions& options)
{
    constexpr auto loadFailed = FlxSessionExitReason::LoadFailed;
    if (currentState != FlxSessionState::Empty)
    {
        lastResult.diagnostics.warning(DiagnosticCode::SessionLoadInvalidState,
            "Unload the session before loading another project.", "session", "load");
        return false;
    }
    lastResult = {};
    const char* stage = "configuration";
    try
    {
        const auto& video = project.context.machine.video;
        if (video.screenWidth <= 0 || video.screenHeight <= 0)
            return fail(DiagnosticCode::InvalidVideoOutputConfiguration, "Logical screen dimensions must be positive", loadFailed, "video");
        impl = std::make_unique<Impl>(project, options);
        stage = "scripting";
        try
        {
            impl->scriptEngine = std::make_unique<ScriptEngine>(options.savePath);
        }
        catch (const std::invalid_argument& error)
        {
            return fail(DiagnosticCode::InvalidSessionConfiguration, error.what(), loadFailed, "savePath");
        }
        if (options.enableVideo)
        {
            stage = "video";
            if (!IsWindowReady())
                return fail(DiagnosticCode::WindowInitializationFailed, "The host must provide a graphics context", loadFailed, stage);
            impl->renderTarget = LoadRenderTexture(video.screenWidth, video.screenHeight);
            if (!impl->renderTarget.id || !impl->renderTarget.texture.id)
                return fail(DiagnosticCode::RenderTargetInitializationFailed, "Session render target could not be initialized", loadFailed, stage);
            SetTextureFilter(impl->renderTarget.texture, video.smoothing ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
            impl->backgroundColor = VideoColorProcessor::project(ColorParser::parse(video.clearColor, BLACK), video);
        }
        stage = "audio";
        impl->audioSystem.configure(impl->context.machine.audio);
        if (options.enableAudio && !impl->audioSystem.attachDevice())
            lastResult.diagnostics.warning(DiagnosticCode::SessionAudioUnavailable,
                "Audio device unavailable. The project will run without sound.", "session", stage);
        stage = "input";
        impl->inputSystem.configure(impl->context.machine.input);
        impl->inputSystem.setMapping(impl->context.inputMapping);
        impl->inputSystem.setPhysicalInputProvider(options.inputProvider);
        stage = "world";
        impl->world = std::make_unique<RuntimeWorld>();
        impl->world->setCollisionDebugEnabled(options.debugCollisions);
        stage = "scripting";
        impl->scriptEngine->setScreenScale(1);
        impl->configureScriptEngine();
        stage = "world";
        const auto loaded = impl->world->load(impl->project, *impl->scriptEngine);
        lastResult.diagnostics.append(loaded.diagnostics);
        if (!loaded.success || loaded.diagnostics.hasErrors())
        {
            if (loaded.diagnostics.hasErrors()) return markFailed(loadFailed);
            return fail(DiagnosticCode::RuntimeWorldLoadFailed, "Runtime world could not be loaded", loadFailed, stage);
        }
        currentState = FlxSessionState::Ready;
        lastResult.success = true;
        if (impl->scriptEngine->exitRequested()) finish(FlxSessionExitReason::ScriptRequestedExit);
        return true;
    }
    catch (const std::exception& error)
    {
        return fail(DiagnosticCode::RuntimeUnexpectedInternalError,
            std::string("Session initialization failed (") + stage + "): " + error.what(), loadFailed, stage);
    }
}

bool FlxSession::update(float deltaTime)
{
    if (currentState != FlxSessionState::Ready && currentState != FlxSessionState::Running) return false;
    if (!std::isfinite(deltaTime) || deltaTime < 0)
        return fail(DiagnosticCode::InvalidSessionDelta, "Session frame delta must be finite and non-negative", FlxSessionExitReason::RuntimeFailed, "update");
    try
    {
        currentState = FlxSessionState::Running;
        impl->scriptEngine->setFrameDelta(deltaTime);
        impl->inputSystem.update(deltaTime);
        impl->world->update(*impl->scriptEngine, deltaTime);
        impl->fadeSystem.update(deltaTime);
        impl->audioSystem.update();
        if (impl->scriptEngine->exitRequested()) finish(FlxSessionExitReason::ScriptRequestedExit);
        else ++lastResult.framesExecuted;
        return true;
    }
    catch (const std::exception& error)
    {
        return fail(DiagnosticCode::RuntimeUnexpectedInternalError, error.what(), FlxSessionExitReason::RuntimeFailed, "update");
    }
}

bool FlxSession::render()
{
    if (!impl || !impl->options.enableVideo ||
        (currentState != FlxSessionState::Ready && currentState != FlxSessionState::Running)) return false;
    if (!IsWindowReady())
        return fail(DiagnosticCode::WindowInitializationFailed, "Host graphics context is unavailable", FlxSessionExitReason::RuntimeFailed, "render");
    try
    {
        BeginTextureMode(impl->renderTarget);
        try
        {
            ClearBackground(impl->backgroundColor);
            const auto& video = impl->context.machine.video;
            impl->world->draw(*impl->scriptEngine, 1, static_cast<float>(video.screenWidth),
                static_cast<float>(video.screenHeight), impl->options.debugCollisions);
            impl->fadeSystem.draw(video.screenWidth, video.screenHeight, 1);
        }
        catch (...) { EndTextureMode(); throw; }
        EndTextureMode();
        if (impl->scriptEngine->exitRequested()) finish(FlxSessionExitReason::ScriptRequestedExit);
        return true;
    }
    catch (const std::exception& error)
    {
        return fail(DiagnosticCode::RuntimeUnexpectedInternalError, error.what(), FlxSessionExitReason::RuntimeFailed, "render");
    }
}

void FlxSession::finish(FlxSessionExitReason reason)
{
    currentState = FlxSessionState::Finished;
    lastResult.exitReason = reason;
    lastResult.success = true;
    impl->audioSystem.shutdown();
}

void FlxSession::requestStop()
{
    if (currentState == FlxSessionState::Ready || currentState == FlxSessionState::Running)
        finish(FlxSessionExitReason::HostRequestedStop);
}

void FlxSession::unload()
{
    impl.reset();
    currentState = FlxSessionState::Empty;
}

FlxSessionState FlxSession::state() const { return currentState; }
bool FlxSession::finished() const { return currentState == FlxSessionState::Finished || currentState == FlxSessionState::Failed; }
const FlxSessionResult& FlxSession::result() const { return lastResult; }
Texture2D FlxSession::output() const { return impl ? impl->renderTarget.texture : Texture2D{}; }

#ifdef FLX_TESTING
std::optional<ScriptValue> FlxSession::testGlobalValue(const std::string& name) const
{
    return impl ? impl->scriptEngine->readGlobalValue(name) : std::nullopt;
}
RuntimeObject* FlxSession::testObject(const std::string& name) { return impl ? impl->world->findByName(name) : nullptr; }
AudioSystem* FlxSession::testAudioSystem() { return impl ? &impl->audioSystem : nullptr; }
#endif
