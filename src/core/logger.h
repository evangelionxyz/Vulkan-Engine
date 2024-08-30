// Copyright (c) 2024, Evangelion Manuhutu
#ifndef LOGGER_H
#define LOGGER_H

#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#   include <format>
#elif PLATFORM_LINUX
#   include <fmt/format.h>
#endif

enum LoggingLevel
{
    Error = 0,
    Info,
    Warning
};

class Logger {
public:
    Logger();

    std::string get_current_time();
    void print_message(LoggingLevel level, const std::string& message);
    void print_formated_message(LoggingLevel level, const std::string& message);
    template <typename... Args> void print_formated_message(LoggingLevel level, fmt::format_string<Args...> format, Args&&... args)
    {
        std::string message = fmt::format(format, std::forward<Args>(args)...);
        print_message(level, message);
    }
    static Logger& get_instance();
};

#define LOG_ERROR(...) Logger::get_instance().print_formated_message(LoggingLevel::Error, __VA_ARGS__)
#define LOG_INFO(...) Logger::get_instance().print_formated_message(LoggingLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) Logger::get_instance().print_formated_message(LoggingLevel::Warning, __VA_ARGS__)

#endif //LOGGER_H
