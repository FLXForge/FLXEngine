#include "SpawnBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
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

        RuntimeObject* source =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (source == nullptr)
        {
            JS_FreeCString(context, spawnName);
            return JS_UNDEFINED;
        }

        const auto it =
            source->childResources.find(spawnName);

        if (it == source->childResources.end())
        {
            Logger::warning(
                "spawn",
                "Child not found: " + std::string(spawnName) +
                " in " + source->runtimeId
            );

            JS_FreeCString(context, spawnName);

            return JS_UNDEFINED;
        }

        Logger::debug(
            "spawn",
            "Child ready: " + std::string(spawnName)
        );

        scriptEngine->spawnObject(
            *source,
            it->second
        );

        JS_FreeCString(context, spawnName);

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
