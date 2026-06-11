#pragma once

struct JSContext;

class AudioBindings
{
public:
    static void registerAll(JSContext* context);
};
