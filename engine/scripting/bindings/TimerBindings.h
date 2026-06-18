#pragma once

#include <quickjs.h>

class TimerBindings
{
public:
    static void registerAll(JSContext* context);
};
