#pragma once

#include "VisualPrimitive.h"

#include <raylib.h>

#include <string>
#include <vector>

class RuntimeObject;

class DrawingContext
{
public:
    void begin(
        RuntimeObject& owner,
        std::vector<VisualPrimitive>& target
    );

    void end();

    bool active() const;
    const RuntimeObject* owner() const;

    bool emitWorldPixel(Vector2 point, Color color);
    bool emitLocalPixel(const RuntimeObject& reference, Vector2 point, Color color);
    bool emitWorldLine(Vector2 start, Vector2 end, Color color);
    bool emitLocalLine(
        const RuntimeObject& reference,
        Vector2 start,
        Vector2 end,
        Color color
    );
    bool emitWorldRectangle(Vector2 center, Vector2 size, Color color);
    bool emitLocalRectangle(
        const RuntimeObject& reference,
        Vector2 center,
        Vector2 size,
        Color color
    );
    bool emitWorldText(
        Vector2 center,
        const std::string& text,
        int fontSize,
        Color color
    );
    bool emitLocalText(
        const RuntimeObject& reference,
        Vector2 center,
        const std::string& text,
        int fontSize,
        Color color
    );

private:
    bool canEmit() const;
    VisualPrimitive basePrimitive(
        VisualPrimitiveKind kind,
        Color color
    ) const;

    RuntimeObject* currentOwner = nullptr;
    std::vector<VisualPrimitive>* primitives = nullptr;
};

