#include "SpawnBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../runtime/ObjectDefinition.h"
#include "../../runtime/RuntimeObject.h"

#include <quickjs.h>

namespace
{
    JSValue jsSpawn(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (argc < 2 || scriptEngine == nullptr)
        {
            return JS_UNDEFINED;
        }

        const char* spawnName =
            JS_ToCString(context, argv[1]);

        if (spawnName == nullptr)
        {
            return JS_UNDEFINED;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* objectId =
            JS_ToCString(context, idValue);

        if (objectId == nullptr)
        {
            JS_FreeCString(context, spawnName);
            JS_FreeValue(context, idValue);
            return JS_UNDEFINED;
        }

        RuntimeObject* source =
            scriptEngine->findObjectByRuntimeId(objectId);

        if (source == nullptr)
        {
            Logger::warning(
                "spawn",
                "Spawner object not found: " + std::string(objectId)
            );

            JS_FreeCString(context, spawnName);
            JS_FreeCString(context, objectId);
            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        auto it =
            source->children.find(spawnName);

        if (it == source->children.end())
        {
            Logger::warning(
                "spawn",
                "Manual child not found: " + std::string(spawnName) +
                " in " + std::string(objectId)
            );

            JS_FreeCString(context, spawnName);
            JS_FreeCString(context, objectId);
            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        const ObjectDefinition& definition =
            it->second;

        if (definition.spawnMode != "manual")
        {
            Logger::warning(
                "spawn",
                "Child is not manual: " + std::string(spawnName)
            );

            JS_FreeCString(context, spawnName);
            JS_FreeCString(context, objectId);
            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        Logger::debug(
            "spawn",
            "Manual child ready: " + definition.id
        );

        scriptEngine->spawnObject(
            *source,
            definition
        );

        JS_FreeCString(context, spawnName);
        JS_FreeCString(context, objectId);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }
}

void SpawnBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        global,
        "spawn",
        JS_NewCFunction(context, jsSpawn, "spawn", 2)
    );

    JS_FreeValue(context, global);
}
