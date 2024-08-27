// Copyright (c) 2024, Evangelion Manuhutu

#ifndef ASSERT_H
#define ASSERT_H

#include <cstdio>

#ifdef _DEBUG
#   ifdef _WIN32
#       define DEBUG_BREAK() __debugbreak()
#   else
#       define DEBUG_BREAK() __builtin_trap()
#   endif
#endif


#ifdef _DEBUG
#   define ASSERT(condition, ...)\
    do {\
        if (!(condition)) {\
            fprintf(stderr, "Assertion failed at '%s' at line %d\n", __FILE__, __LINE__);\
            fprintf(stderr, __VA_ARGS__);\
            fprintf(stderr,"\n\n");\
            DEBUG_BREAK();\
        }\
    } while(0)
#else
#   define ASSERT(condition, ...)
#endif

#endif //ASSERT_H
