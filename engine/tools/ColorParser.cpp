#include "ColorParser.h"
#include "TextTools.h"
#include "../debug/Logger.h"

#include <optional>
#include <unordered_map>

namespace
{
    std::optional<int> hexValue(char value)
    {
        if (value >= '0' && value <= '9')
        {
            return value - '0';
        }

        if (value >= 'a' && value <= 'f')
        {
            return 10 + value - 'a';
        }

        if (value >= 'A' && value <= 'F')
        {
            return 10 + value - 'A';
        }

        return std::nullopt;
    }

    std::optional<Color> parseHexColor(const std::string& value)
    {
        if (value.size() != 4 && value.size() != 7)
        {
            return std::nullopt;
        }

        if (value[0] != '#')
        {
            return std::nullopt;
        }

        auto readDigit = [](char digit) -> std::optional<unsigned char>
        {
            const auto parsed = hexValue(digit);

            if (!parsed.has_value())
            {
                return std::nullopt;
            }

            return static_cast<unsigned char>(
                parsed.value() * 17
            );
        };

        auto readByte = [](char high, char low) -> std::optional<unsigned char>
        {
            const auto parsedHigh = hexValue(high);
            const auto parsedLow = hexValue(low);

            if (!parsedHigh.has_value() || !parsedLow.has_value())
            {
                return std::nullopt;
            }

            return static_cast<unsigned char>(
                parsedHigh.value() * 16 + parsedLow.value()
            );
        };

        std::optional<unsigned char> r;
        std::optional<unsigned char> g;
        std::optional<unsigned char> b;

        if (value.size() == 4)
        {
            r = readDigit(value[1]);
            g = readDigit(value[2]);
            b = readDigit(value[3]);
        }
        else
        {
            r = readByte(value[1], value[2]);
            g = readByte(value[3], value[4]);
            b = readByte(value[5], value[6]);
        }

        if (!r.has_value() || !g.has_value() || !b.has_value())
        {
            return std::nullopt;
        }

        return Color{
            r.value(),
            g.value(),
            b.value(),
            255
        };
    }
}

Color ColorParser::parse(
    const std::string& value,
    Color fallback
)
{
    const auto hexColor =
        parseHexColor(value);

    if (hexColor.has_value())
    {
        return hexColor.value();
    }

    const std::string colorName =
        TextTools::toLower(value);

    static const std::unordered_map<std::string, Color> colors = {
        { "lightgray", LIGHTGRAY },
        { "lightgrey", LIGHTGRAY },
        { "gray", GRAY },
        { "grey", GRAY },
        { "darkgray", DARKGRAY },
        { "darkgrey", DARKGRAY },
        { "yellow", YELLOW },
        { "gold", GOLD },
        { "orange", ORANGE },
        { "pink", PINK },
        { "red", RED },
        { "maroon", MAROON },
        { "green", GREEN },
        { "lime", LIME },
        { "darkgreen", DARKGREEN },
        { "skyblue", SKYBLUE },
        { "blue", BLUE },
        { "darkblue", DARKBLUE },
        { "purple", PURPLE },
        { "violet", VIOLET },
        { "darkpurple", DARKPURPLE },
        { "beige", BEIGE },
        { "brown", BROWN },
        { "darkbrown", DARKBROWN },
        { "white", WHITE },
        { "black", BLACK },
        { "blank", BLANK },
        { "magenta", MAGENTA },
        { "raywhite", RAYWHITE }
    };

    const auto it =
        colors.find(colorName);

    if (it != colors.end())
    {
        return it->second;
    }

    Logger::warning(
        "json",
        "Unknown color '" + value + "', using fallback"
    );

    return fallback;
}
