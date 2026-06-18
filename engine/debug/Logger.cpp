#include "Logger.h"

#include <iostream>
#include <raylib.h>

namespace
{
    bool debugEnabled = false;
}

void Logger::setDebugEnabled(bool enabled)
{
    debugEnabled = enabled;
}

void Logger::info(
    const std::string& category,
    const std::string& message
)
{
    std::cout
        << "<<INFO:[" << category << "] >>:: "
        << message
        << std::endl;
}

void Logger::warning(
    const std::string& category,
    const std::string& message
)
{
    std::cout
        << "<<WARNING:[" << category << "] >>:: "
        << message
        << std::endl;
}

void Logger::error(
    const std::string& category,
    const std::string& message
)
{
    std::cout
        << "<<ERROR:[" << category << "] >>:: "
        << message
        << std::endl;
}

void Logger::debug(
    const std::string& message
)
{
    if (!debugEnabled)
    {
        return;
    }

    std::cout
        << "<<DEBUG: >>:: "
        << message
        << std::endl;
}

void Logger::debug(
    const std::string& category,
    const std::string& message
)
{
    if (!debugEnabled)
    {
        return;
    }

    std::cout
        << "<<DEBUG:[" << category << "] >>:: "
        << message
        << std::endl;
}

void Logger::rayLibLog(int msgType, const char *text, va_list args) {
    switch (msgType) {
        case LOG_INFO:    printf("<<INFO:[graphics] >>:: "); break;
        case LOG_ERROR:   printf("<<ERROR:[graphics] >>:: "); break;
        case LOG_WARNING: printf("<<WARNING:[graphics] >>:: "); break;
        case LOG_DEBUG:   printf("<<DEBUG: >>:: "); break;
        default: break;
    }
    
    vprintf(text, args);
    printf("\n");
}
