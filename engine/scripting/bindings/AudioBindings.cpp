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

    JSValue jsPlayMusic(
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

        const char* musicId =
            JS_ToCString(context, argv[1]);

        if (objectId == nullptr || musicId == nullptr)
        {
            if (objectId != nullptr)
            {
                JS_FreeCString(context, objectId);
            }

            if (musicId != nullptr)
            {
                JS_FreeCString(context, musicId);
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
                "Music source object not found: " + std::string(objectId)
            );

            JS_FreeCString(context, objectId);
            JS_FreeCString(context, musicId);
            JS_FreeValue(context, idValue);

            return JS_UNDEFINED;
        }

        scriptEngine->playMusic(
            *source,
            musicId
        );

        JS_FreeCString(context, objectId);
        JS_FreeCString(context, musicId);
        JS_FreeValue(context, idValue);

        return JS_UNDEFINED;
    }

    JSValue jsStopMusic(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine != nullptr)
        {
            scriptEngine->stopMusic();
        }

        return JS_UNDEFINED;
    }

    JSValue jsPauseMusic(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine != nullptr)
        {
            scriptEngine->pauseMusic();
        }

        return JS_UNDEFINED;
    }

    JSValue jsMusicActive(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        return JS_NewBool(
            context,
            scriptEngine != nullptr && scriptEngine->musicActive()
        );
    }

    JSValue jsMusicPaused(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        return JS_NewBool(
            context,
            scriptEngine != nullptr && scriptEngine->musicPaused()
        );
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

    JS_SetPropertyStr(
        context,
        global,
        "play_music",
        JS_NewCFunction(context, jsPlayMusic, "play_music", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "stop_music",
        JS_NewCFunction(context, jsStopMusic, "stop_music", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "pause_music",
        JS_NewCFunction(context, jsPauseMusic, "pause_music", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "music_active",
        JS_NewCFunction(context, jsMusicActive, "music_active", 0)
    );

    JS_SetPropertyStr(
        context,
        global,
        "music_paused",
        JS_NewCFunction(context, jsMusicPaused, "music_paused", 0)
    );

    JS_FreeValue(context, global);
}
