#include "TextTools.h"

#include <algorithm>
#include <cctype>

std::string TextTools::toLower(const std::string& value)
{
    std::string result = value;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        }
    );

    return result;
}
