#pragma once

struct JSContext;

class CoreBindings
{
public:
    static void registerAll(JSContext* context);
};
