#include "ScriptBindings.h"
#include "ScriptEngine.h"
#include "bindings/AudioBindings.h"
#include "bindings/CoreBindings.h"
#include "bindings/DrawBindings.h"
#include "bindings/InputBindings.h"
#include "bindings/MotionBindings.h"
#include "bindings/SpawnBindings.h"
#include "bindings/StateBindings.h"
#include "bindings/TimerBindings.h"

#include <quickjs.h>

void ScriptBindings::registerAll(
    JSContext* context,
    ScriptEngine* scriptEngine
)
{
    JS_SetContextOpaque(context, scriptEngine);

    CoreBindings::registerAll(context);
    InputBindings::registerAll(context);
    MotionBindings::registerAll(context);
    DrawBindings::registerAll(context);
    AudioBindings::registerAll(context);
    SpawnBindings::registerAll(context);
    StateBindings::registerAll(context);
    TimerBindings::registerAll(context);
}
