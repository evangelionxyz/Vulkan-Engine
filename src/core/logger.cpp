// Copyright (c) 2024, Evangelion Manuhutu
#include "logger.h"

static Logger *s_Instance = nullptr;
Logger::Logger()
{
    s_Instance = this;
}

std::string Logger::get_current_time()
{
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = *std::localtime(&time_t_now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "[%H:%M:%S]");
    return oss.str();
}

void Logger::print_message(LoggingLevel level, const std::string& message)
{
    switch (level)
    {
    case LoggingLevel::Error:
        fprintf(stderr, "%s [ERROR] %s\n", get_current_time().c_str(), message.c_str());
        break;
    case LoggingLevel::Warning:
        fprintf(stdout, "%s [WARNING] %s\n", get_current_time().c_str(), message.c_str());
        break;
    case LoggingLevel::Info:
        fprintf(stdout, "%s [INFO] %s\n", get_current_time().c_str(), message.c_str());
        break;
    }
}

void Logger::print_formated_message(LoggingLevel level, const std::string& message)
{
    print_message(level, message);
}

Logger& Logger::get_instance()
{
    return *s_Instance;
}