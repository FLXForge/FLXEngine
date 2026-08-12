#include "TimerBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"
#include "../../runtime/RuntimeObject.h"

#include <cmath>
#include <quickjs.h>
#include <string>

namespace
{
    RuntimeObject* objectFromArgument(
        JSContext* context,
        JSValueConst value
    )
    {
        ScriptEngine* scriptEngine =
            scriptEngineFromContext(context);

        if (scriptEngine == nullptr)
        {
            return nullptr;
        }

        JSValue idValue =
            JS_GetPropertyStr(context, value, "id");

        const char* id =
            JS_ToCString(context, idValue);

        if (id == nullptr)
        {
            JS_FreeValue(context, idValue);
            return nullptr;
        }

        RuntimeObject* object =
            scriptEngine->findObjectByRuntimeId(id);

        JS_FreeCString(context, id);
        JS_FreeValue(context, idValue);

        return object;
    }

    bool timerNameFromArgument(
        JSContext* context,
        JSValueConst value,
        std::string& outTimerName
    )
    {
        if (!JS_IsString(value))
        {
            Logger::warning(
                "timer",
                "Timer name must be a non-empty string"
            );

            return false;
        }

        const char* timerName =
            JS_ToCString(context, value);

        if (timerName == nullptr)
        {
            return false;
        }

        outTimerName =
            timerName;

        JS_FreeCString(context, timerName);

        if (outTimerName.empty())
        {
            Logger::warning(
                "timer",
                "Timer name must be a non-empty string"
            );

            return false;
        }

        return true;
    }

    bool durationFromArgument(
        JSContext* context,
        JSValueConst value,
        float& outDuration
    )
    {
        if (!JS_IsNumber(value))
        {
            Logger::warning(
                "timer",
                "Timer duration must be a finite number greater than 0"
            );

            return false;
        }

        double duration = 0.0;

        if (JS_ToFloat64(context, &duration, value) != 0)
        {
            Logger::warning(
                "timer",
                "Timer duration must be a finite number greater than 0"
            );

            return false;
        }

        if (!std::isfinite(duration) || duration <= 0.0)
        {
            Logger::warning(
                "timer",
                "Timer duration must be a finite number greater than 0"
            );

            return false;
        }

        outDuration =
            static_cast<float>(duration);

        return true;
    }

    void playExistingTimerWithDuration(
        RuntimeTimer& timer,
        float duration
    )
    {
        if (timer.status == RuntimeTimerStatus::Done)
        {
            timer.duration = duration;
            timer.left = duration;
            timer.status = RuntimeTimerStatus::Running;
            return;
        }

        const float elapsed =
            timer.duration - timer.left;

        const float newLeft =
            duration - elapsed;

        timer.duration =
            duration;

        if (newLeft <= 0.0f)
        {
            timer.left = 0.0f;
            timer.status = RuntimeTimerStatus::Done;
            return;
        }

        timer.left =
            newLeft;
        timer.status =
            RuntimeTimerStatus::Running;
    }

    JSValue jsPlayTimer(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "play_timer requires object and timer name"
            );

            return JS_UNDEFINED;
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        if (object == nullptr || !hasTimerName)
        {
            return JS_UNDEFINED;
        }

        auto it =
            object->timers.find(timerName);

        if (argc < 3)
        {
            if (it == object->timers.end())
            {
                Logger::warning(
                    "timer",
                    "Cannot play timer without duration because it does not exist: " +
                    timerName
                );

                return JS_UNDEFINED;
            }

            if (it->second.status == RuntimeTimerStatus::Paused)
            {
                it->second.status =
                    RuntimeTimerStatus::Running;
            }
            else if (it->second.status == RuntimeTimerStatus::Done)
            {
                it->second.left =
                    it->second.duration;
                it->second.status =
                    RuntimeTimerStatus::Running;
            }

            return JS_UNDEFINED;
        }

        float duration = 0.0f;

        if (!durationFromArgument(context, argv[2], duration))
        {
            return JS_UNDEFINED;
        }

        if (it == object->timers.end())
        {
            object->timers[timerName] = RuntimeTimer{
                duration,
                duration,
                RuntimeTimerStatus::Running
            };

            return JS_UNDEFINED;
        }

        playExistingTimerWithDuration(
            it->second,
            duration
        );

        return JS_UNDEFINED;
    }

    JSValue jsPauseTimer(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "pause_timer requires object and timer name"
            );

            return JS_UNDEFINED;
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        if (object == nullptr || !hasTimerName)
        {
            return JS_UNDEFINED;
        }

        auto it =
            object->timers.find(timerName);

        if (
            it != object->timers.end() &&
            it->second.status == RuntimeTimerStatus::Running
            )
        {
            it->second.status =
                RuntimeTimerStatus::Paused;
        }

        return JS_UNDEFINED;
    }

    JSValue jsStopTimer(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "stop_timer requires object and timer name"
            );

            return JS_UNDEFINED;
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        if (object != nullptr && hasTimerName)
        {
            object->timers.erase(timerName);
        }

        return JS_UNDEFINED;
    }

    JSValue jsTimerActive(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "timer_active requires object and timer name"
            );

            return JS_NewBool(context, false);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        bool active =
            false;

        if (object != nullptr && hasTimerName)
        {
            const auto it =
                object->timers.find(timerName);

            active =
                it != object->timers.end() &&
                (
                    it->second.status == RuntimeTimerStatus::Running ||
                    it->second.status == RuntimeTimerStatus::Paused
                );
        }

        return JS_NewBool(context, active);
    }

    JSValue jsTimerPaused(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "timer_paused requires object and timer name"
            );

            return JS_NewBool(context, false);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        bool paused =
            false;

        if (object != nullptr && hasTimerName)
        {
            const auto it =
                object->timers.find(timerName);

            paused =
                it != object->timers.end() &&
                it->second.status == RuntimeTimerStatus::Paused;
        }

        return JS_NewBool(context, paused);
    }

    JSValue jsTimerDone(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "timer_done requires object and timer name"
            );

            return JS_NewBool(context, false);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        bool done =
            false;

        if (object != nullptr && hasTimerName)
        {
            const auto it =
                object->timers.find(timerName);

            done =
                it != object->timers.end() &&
                it->second.status == RuntimeTimerStatus::Done;
        }

        return JS_NewBool(context, done);
    }

    JSValue jsTimerLeft(
        JSContext* context,
        JSValueConst thisValue,
        int argc,
        JSValueConst* argv
    )
    {
        if (argc < 2)
        {
            Logger::warning(
                "timer",
                "timer_left requires object and timer name"
            );

            return JS_NewFloat64(context, 0.0);
        }

        RuntimeObject* object =
            objectFromArgument(context, argv[0]);

        std::string timerName;
        const bool hasTimerName =
            timerNameFromArgument(context, argv[1], timerName);

        double left =
            0.0;

        if (object != nullptr && hasTimerName)
        {
            const auto it =
                object->timers.find(timerName);

            if (it != object->timers.end())
            {
                left =
                    it->second.left;
            }
        }

        return JS_NewFloat64(context, left);
    }
}

void TimerBindings::registerAll(JSContext* context)
{
    JSValue global =
        JS_GetGlobalObject(context);

    JS_SetPropertyStr(
        context,
        global,
        "play_timer",
        JS_NewCFunction(context, jsPlayTimer, "play_timer", 3)
    );

    JS_SetPropertyStr(
        context,
        global,
        "pause_timer",
        JS_NewCFunction(context, jsPauseTimer, "pause_timer", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "stop_timer",
        JS_NewCFunction(context, jsStopTimer, "stop_timer", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_active",
        JS_NewCFunction(context, jsTimerActive, "timer_active", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_paused",
        JS_NewCFunction(context, jsTimerPaused, "timer_paused", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_done",
        JS_NewCFunction(context, jsTimerDone, "timer_done", 2)
    );

    JS_SetPropertyStr(
        context,
        global,
        "timer_left",
        JS_NewCFunction(context, jsTimerLeft, "timer_left", 2)
    );

    JS_FreeValue(context, global);
}
