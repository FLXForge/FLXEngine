#include "FadeSystem.h"
#include "../debug/Logger.h"

#include <algorithm>

void FadeSystem::fadeOn(Color nextColor)
{
    begin(Direction::On, nextColor);
}

void FadeSystem::fadeOff(Color nextColor)
{
    begin(Direction::Off, nextColor);
}

void FadeSystem::begin(
    Direction nextDirection,
    Color nextColor
)
{
    direction = nextDirection;
    active = true;
    color = nextColor;
    startAlpha = alpha;
    elapsed = 0.0f;

    if (direction == Direction::On)
    {
        targetAlpha = 1.0f;
        Logger::info("graphics", "fade_on iniciado");
    }
    else
    {
        targetAlpha = 0.0f;
        Logger::info("graphics", "fade_off iniciado");
    }
}

void FadeSystem::set(
    float nextAlpha,
    Color nextColor
)
{
    alpha =
        std::clamp(nextAlpha, 0.0f, 1.0f);

    startAlpha = alpha;
    targetAlpha = alpha;
    elapsed = 0.0f;
    active = false;
    direction = Direction::None;
    color = nextColor;

    Logger::info(
        "graphics",
        "fade_set alpha=" + std::to_string(alpha)
    );
}

bool FadeSystem::isActive() const
{
    return active;
}

bool FadeSystem::isDone() const
{
    return !active;
}

float FadeSystem::getAlpha() const
{
    return alpha;
}

void FadeSystem::update(float delta)
{
    if (!active || direction == Direction::None)
    {
        return;
    }

    elapsed += delta;

    const float progress =
        std::clamp(elapsed / duration, 0.0f, 1.0f);

    alpha =
        startAlpha +
        (targetAlpha - startAlpha) * progress;

    if (progress < 1.0f)
    {
        return;
    }

    alpha = targetAlpha;

    if (direction == Direction::On)
    {
        Logger::info("graphics", "fade_on completado");
        direction = Direction::None;
        active = false;
    }
    else
    {
        Logger::info("graphics", "fade_off completado");
        direction = Direction::None;
        active = false;
    }
}

void FadeSystem::draw(
    int screenWidth,
    int screenHeight,
    int screenScale
) const
{
    if (alpha <= 0.0f)
    {
        return;
    }

    Color overlay = color;
    overlay.a =
        static_cast<unsigned char>(
            std::clamp(alpha, 0.0f, 1.0f) * 255.0f
        );

    DrawRectangle(
        0,
        0,
        screenWidth * screenScale,
        screenHeight * screenScale,
        overlay
    );
}
