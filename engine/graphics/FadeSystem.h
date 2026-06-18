#pragma once

#include <raylib.h>

#include <string>

class FadeSystem
{
public:
    void fadeOn(const std::string& color);
    void fadeOff(const std::string& color);
    void set(
        float alpha,
        const std::string& color
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
        const std::string& color
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
