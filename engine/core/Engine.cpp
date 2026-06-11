#include "Engine.h"
#include "../debug/Logger.h"
#include "../loading/JsonLoader.h"
#include "../project/FlxContextBuilder.h"

#include <raylib.h>

Engine::Engine() = default;

void Engine::run(const std::string& flxPath)
{
    SetTraceLogCallback(Logger::rayLibLog);

    init(flxPath);

    while (!WindowShouldClose())
    {
        update();
        draw();
    }

    shutdown();
}

void Engine::init(const std::string& flxPath)
{
    loadProject(flxPath);
    initWindow();
    configureScriptEngine();

    SetTargetFPS(60);
}

void Engine::loadProject(const std::string& flxPath)
{
    context =
        FlxContextBuilder::build(flxPath);

    Logger::setDebugEnabled(context.debugLogs);

    Logger::info(
        "project",
        "Loaded project: " + context.name
    );

    const std::string rootPath =
        JsonLoader::resolveProjectPath(
            context.projectPath,
            context.root,
            ".json"
        );

    Logger::info(
        "project",
        "Loaded root: " + rootPath
    );

    const ObjectDefinition rootDefinition =
        JsonLoader::loadObjectDefinition(rootPath);

    world.load(rootDefinition, scriptEngine);

    scriptEngine.setScreenScale(context.screenScale);
}

void Engine::initWindow()
{
    const std::string title =
        context.screenTitle.empty()
        ? "Flx"
        : context.screenTitle;

    InitWindow(
        context.screenWidth * context.screenScale,
        context.screenHeight * context.screenScale,
        title.c_str()
    );
}

void Engine::configureScriptEngine()
{
    scriptEngine.setFindObjectFunction(
        [this](const std::string& name)
        {
            return world.findByName(name);
        }
    );

    scriptEngine.setFindObjectByIdFunction(
        [this](const std::string& id)
        {
            return world.findByRuntimeId(id);
        }
    );

    scriptEngine.setSpawnObjectFunction(
        [this](
            RuntimeObject& source,
            const ObjectDefinition& definition
            )
        {
            world.spawn(
                source,
                definition,
                scriptEngine
            );
        }
    );
}

void Engine::update()
{
    world.update(
        scriptEngine,
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight)
    );
}

void Engine::draw()
{
    BeginDrawing();

    ClearBackground(BLACK);

    world.draw(
        scriptEngine,
        context.screenScale,
        static_cast<float>(context.screenWidth),
        static_cast<float>(context.screenHeight),
        context.debugCollisions
    );

    EndDrawing();
}

void Engine::shutdown()
{
    CloseWindow();
}

RuntimeObject* Engine::find(const std::string& name)
{
    return world.findByName(name);
}
