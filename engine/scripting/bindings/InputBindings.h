#pragma once

struct JSContext;

class InputBindings
{
public:
    static void registerAll(JSContext* context);
};
