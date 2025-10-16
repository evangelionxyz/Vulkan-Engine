// Copyright (c) 2024, Evangelion Manuhutu

#ifndef ASSERT_HPP
#define ASSERT_HPP

#include <cstdio>
#include "logger.hpp"

#ifdef VK_DEBUG
#   ifdef _WIN32
#       define DEBUG_BREAK() __debugbreak()
#   else
#       define DEBUG_BREAK() __builtin_trap()
#   endif
#else
#       define DEBUG_BREAK()
#endif

#ifdef VK_DEBUG
#   define ASSERT(condition, ...)\
    do {\
        if (!(condition)) {\
            Logger::get_instance().push_message(LoggingLevel::Error, "Assertion failed at {} at line {}:\n{}", __FILE__, __LINE__, __VA_ARGS__);\
            DEBUG_BREAK();\
        }\
    } while(0)
#else
#   define ASSERT(...)
#endif

#endif //ASSERT_H
