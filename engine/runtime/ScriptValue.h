#pragma once

#include <string>
#include <variant>

using ScriptValue =
    std::variant<double, bool, std::string>;
