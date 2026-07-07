#pragma once

#include <string>
#include <raylib.h>

class Logger
{
public:

    static void setConsoleEnabled(bool enabled);
    static void setDebugEnabled(bool enabled);

    static void info(
        const std::string& category,
        const std::string& message
    );

    static void warning(
        const std::string& category,
        const std::string& message
    );

    static void error(
        const std::string& category,
        const std::string& message
    );

    static void debug(
        const std::string& message
    );

    static void debug(
        const std::string& category,
        const std::string& message
    );

    static void rayLibLog(
        int msgType, 
        const char* text, 
        va_list args
    );
};
