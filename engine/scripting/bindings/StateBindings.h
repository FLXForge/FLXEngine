#pragma once

#include <quickjs.h>

class StateBindings
{
public:
    static void registerAll(JSContext* context);
};
