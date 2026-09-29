#pragma once

#include <raylib.h>

class FadeSystem
{
public:
    void fadeOn(Color color);
    void fadeOff(Color color);
    void set(
        float alpha,
        Color color
    );

    bool isActive() const;
    bool isDone() const;
    float getAlpha() const;

    void update(float delta);

    void draw(
        int screenWidth,
        int screenHeight,
        int screenScale
    ) const;

private:
    enum class Direction
    {
        None,
        On,
        Off
    };

    void begin(
        Direction nextDirection,
        Color color
    );

    bool active = false;
    Direction direction = Direction::None;
    Color color = BLACK;
    float alpha = 0.0f;
    float startAlpha = 0.0f;
    float targetAlpha = 0.0f;
    float elapsed = 0.0f;
    float duration = 1.0f;
};
