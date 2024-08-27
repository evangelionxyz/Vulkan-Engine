// Copyright (c) 2024, Evangelion Manuhutu

#ifndef ASSERT_H
#define ASSERT_H

#include <cstdio>
#include "logger.h"

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
            LOG_ERROR("Assertion failed at '{0}' at line {1}\n", __FILE__, __LINE__);\
            LOG_ERROR(__VA_ARGS__);\
            DEBUG_BREAK();\
        }\
    } while(0)
#else
#   define ASSERT(...)
#endif

#endif //ASSERT_H
