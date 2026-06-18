#include "AudioBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../runtime/RuntimeObject.h"

#include <quickjs.h>

namespace
{
    JSValue jsPlaySound(
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

        JSValue idValue =
            JS_GetPropertyStr(context, argv[0], "id");

        const char* objectId =
            JS_ToCString(context, idValue);

        const char* soundId =
            JS_ToCString(context, argv[1]);

        if (objectId == nullptr || soundId == nullptr)
        {
            if (objectId != nullptr)
            {
                JS_FreeCString(context, objectId);
            }

            if (soundId != nullptr)
            {
                JS_FreeCString(context, soundId);
            }

            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        RuntimeObject* source =
            scriptEngine->findObjectByRuntimeId(objectId);

        if (source == nullptr)
        {
            Logger::warning(
                "audio",
                "Sound source object not found: " + std::string(objectId)
            );

            JS_FreeCString(context, objectId);
            JS_FreeCString(context, soundId);
            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->playSound(
            *source,
            soundId
        );

        JS_FreeCString(context, objectId);
        JS_FreeCString(context, soundId);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }
}

void AudioBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        global,
        "play_sound",
        JS_NewCFunction(context, jsPlaySound, "play_sound", 2)
    );

    JS_FreeValue(context, global);
}
