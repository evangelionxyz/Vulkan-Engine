// Copyright (c) 2024, Evangelion Manuhutu
#ifndef LOGGER_H
#define LOGGER_H

#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utility>
#include <iostream>

enum LoggingLevel
{
    Error = 0,
    Info,
    Warning
};

struct LogMessage
{
    std::string message;
    LoggingLevel level;
    LogMessage() = default;
    LogMessage(std::string msg, LoggingLevel lvl)
        : message(std::move(msg)), level(lvl) {}
};

class Logger {
public:
    Logger()
    {
        std::ios::sync_with_stdio(false);
        std::cin.tie(nullptr);
    }

    void push_message(std::string message, LoggingLevel level = LoggingLevel::Info)
    {
        if (m_Messages.size() > 1024)
            m_Messages.erase(m_Messages.begin());

        m_Messages.emplace_back(std::move(message), level);
        print_colored_message(m_Messages.back().message, level);
    }

    template<typename... Args>
    void push_message(LoggingLevel level, const char *format, Args&&... args)
    {
        std::string message = format_string(format, std::forward<Args>(args)...);
        push_message(std::move(message), level);
    }

    void clear_messages()
    {
        m_Messages.clear();
    }

    const std::vector<LogMessage> &get_messages() const
    {
        return m_Messages;
    }

    static Logger &get_instance()
    {
        static Logger instance;
        return instance;
    }

    std::string get_current_time()
    {
        const auto now = std::chrono::system_clock::now();
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);
        const std::tm tm = *std::localtime(&time_t_now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "[%H:%M:%S]");
        return oss.str();
    }

private:
    void print_colored_message(const std::string &message, LoggingLevel level)
    {
        const char *color_code;
        const char *level_str;

        switch (level)
        {
            case LoggingLevel::Info:
                color_code = "\033[1;37m";
                level_str = "[Info]";
                break;
            case LoggingLevel::Error:
                color_code = "\033[1;31m";
                level_str = "[Error]";
                break;
            case LoggingLevel::Warning:
                color_code = "\033[1;33m";
                level_str = "[Warning]";
                break;
        }

        if (m_Buffer.tellp() > 1024)
        {
            m_Buffer.str("");
            m_Buffer.clear();
        }

        m_Buffer << color_code << level_str << message << "\033[0m\n";
        std::cout << m_Buffer.rdbuf();
    }

    template<typename... Args>
    static std::string format_string(const char *format, Args&&... args)
    {
        std::ostringstream ss;
        format_string_impl(ss, format, std::forward<Args>(args)...);
        return ss.str();
    }

    static void format_string_impl(std::ostringstream &ss, const char *format)
    {
        ss << format;
    }

    template<typename T, typename... Args>
    static void format_string_impl(std::ostringstream &ss, const char *format, T&& value, Args&&... args)
    {
        while (*format)
        {
            if (*format == '{' && *(format + 1) == '}')
            {
                ss << value;
                format_string_impl(ss, format + 1, std::forward<Args>(args)...);
                return;
            }

            ss << *format++;
        }
    }

    std::vector<LogMessage> m_Messages;
    std::stringstream m_Buffer;
};

#define LOG_ERROR(format, ...) Logger::get_instance().push_message(LoggingLevel::Error, format, __VA_ARGS__)
#define LOG_INFO(format, ...) Logger::get_instance().push_message(LoggingLevel::Info, format, __VA_ARGS__)
#define LOG_WARN(format, ...) Logger::get_instance().push_message(LoggingLevel::Warning, format, __VA_ARGS__)

#endif //LOGGER_H
