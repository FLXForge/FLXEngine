#pragma once

struct JSContext;

class DrawBindings
{
public:
    static void registerAll(JSContext* context);
};
