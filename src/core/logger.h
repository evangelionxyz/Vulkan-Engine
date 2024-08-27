// Copyright (c) 2024, Evangelion Manuhutu
#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

class Logger {
public:
    static void init();
    static std::shared_ptr<spdlog::logger> &get_core_logger() { return s_CoreLogger; }
private:
    static std::shared_ptr<spdlog::logger> s_CoreLogger;
};

#define LOG_ERROR(...)     Logger::get_core_logger()->error(__VA_ARGS__)
#define LOG_WARN(...)      Logger::get_core_logger()->warn(__VA_ARGS__)
#define LOG_INFO(...)      Logger::get_core_logger()->info(__VA_ARGS__)
#define LOG_TRACE(...)     Logger::get_core_logger()->trace(__VA_ARGS__)
#define LOG_CRITICAL(...)	Logger::get_core_logger()->critical(__VA_ARGS__)

#endif //LOGGER_H
