// Copyright (c) 2024, Evangelion Manuhutu
#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;

void Logger::init()
{
    spdlog::set_pattern("%^[%l] %v");
    s_CoreLogger = spdlog::stdout_color_mt("Vulkan Core");
    s_CoreLogger->set_level(spdlog::level::trace);
}
