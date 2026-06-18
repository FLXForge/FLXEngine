#pragma once

struct JSContext;

class SpawnBindings
{
public:
    static void registerAll(JSContext* context);
};
