#include "AnimationBindings.h"
#include "BindingHelpers.h"
#include "../../debug/Logger.h"

namespace
{
    void invalid(const char* message)
    {
        Logger::warning("animation", std::string(diagnosticCodeText(DiagnosticCode::InvalidAnimationOperation)) + " " + message);
    }

    void report(const Diagnostics& diagnostics)
    {
        for (const auto& diagnostic : diagnostics.all())
            Logger::warning("animation", std::string(diagnosticCodeText(diagnostic.code)) + " " + diagnostic.message);
    }

    // The common helper enforces invocation-scoped views, including dead hooks.
    RuntimeObject* objectArgument(JSContext* context, int argc, JSValueConst* argv)
    {
        RuntimeObject* object = argc > 0 ? runtimeObjectViewFromArgument(context, argv[0]) : nullptr;
        if (!object) invalid("Animation requires a valid RuntimeObject view from the current callback");
        return object;
    }

    JSValue play(JSContext* context, JSValueConst, int argc, JSValueConst* argv)
    {
        RuntimeObject* object = objectArgument(context, argc, argv);
        if (!object) return JS_UNDEFINED;
        if (argc < 2 || !JS_IsString(argv[1]))
        {
            invalid("play_animation requires a non-empty animation name");
            return JS_UNDEFINED;
        }
        size_t length = 0;
        const char* text = JS_ToCStringLen(context, &length, argv[1]);
        if (!text) return JS_EXCEPTION;
        const std::string name(text, length);
        JS_FreeCString(context, text);
        const auto* definition = scriptEngineFromContext(context)->findObjectDefinition(object->definitionId);
        if (!definition || name.empty())
        {
            invalid("Animation name must identify a declared animation");
            return JS_UNDEFINED;
        }
        const auto base = AnimationBaseAppearance::capture(object->representation,
            object->hasVisualColor ? object->visualColor : WHITE, object->hasVisualColor);
        Diagnostics diagnostics;
        object->animation.play(name, definition->animations, base, diagnostics);
        report(diagnostics);
        return JS_UNDEFINED;
    }

    enum Operation { Pause, Stop, Current, Active, Paused, Done, Left };
    JSValue operation(JSContext* context, JSValueConst, int argc, JSValueConst* argv, int magic)
    {
        RuntimeObject* object = objectArgument(context, argc, argv);
        switch (magic)
        {
        case Pause: if (object) object->animation.pause(); return JS_UNDEFINED;
        case Stop: if (object) object->animation.stop(); return JS_UNDEFINED;
        case Current:
            if (object)
                if (const auto name = object->animation.current())
                    return JS_NewStringLen(context, name->data(), name->size());
            return JS_NULL;
        case Active: return JS_NewBool(context, object && object->animation.active());
        case Paused: return JS_NewBool(context, object && object->animation.paused());
        case Done: return JS_NewBool(context, object && object->animation.done());
        case Left: return JS_NewFloat64(context, object ? object->animation.left() : 0.0);
        default: return JS_UNDEFINED;
        }
    }
}

void AnimationBindings::registerAll(JSContext* context)
{
    JSValue global = JS_GetGlobalObject(context);
    JS_SetPropertyStr(context, global, "play_animation", JS_NewCFunction(context, play, "play_animation", 2));
    const struct { const char* name; Operation op; } functions[] = {
        {"animation_active", Active}, {"animation_current", Current},
        {"animation_done", Done}, {"animation_left", Left}, {"animation_paused", Paused},
        {"pause_animation", Pause}, {"stop_animation", Stop}
    };
    for (const auto& function : functions)
        JS_SetPropertyStr(context, global, function.name,
            JS_NewCFunctionMagic(context, operation, function.name, 1, JS_CFUNC_generic_magic, function.op));
    JS_FreeValue(context, global);
}
