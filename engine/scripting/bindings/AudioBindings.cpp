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

        const char* soundId =
            JS_ToCString(context, argv[1]);

        RuntimeObject* source =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (source == nullptr || soundId == nullptr)
        {
            if (soundId != nullptr)
            {
                JS_FreeCString(context, soundId);
            }

            return JS_UNDEFINED;
        }

        scriptEngine->playSound(
            *source,
            soundId
        );

        JS_FreeCString(context, soundId);

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

        const char* musicId =
            JS_ToCString(context, argv[1]);

        RuntimeObject* source =
            runtimeObjectViewFromArgument(context, argv[0]);

        if (source == nullptr || musicId == nullptr)
        {
            if (musicId != nullptr)
            {
                JS_FreeCString(context, musicId);
            }

            return JS_UNDEFINED;
        }

        scriptEngine->playMusic(
            *source,
            musicId
        );

        JS_FreeCString(context, musicId);

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
