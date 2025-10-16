#ifndef LIB_H
#define LIB_H

#ifdef VULKAN_SHARED_BUILD
    #ifdef _WIN32
        #define VULKAN_DLL_API __declspec(dllexport)
    #elif __linux__
        #define VULKAN_DLL_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define VULKAN_DLL_API __declspec(dllimport)
    #elif __linux__
        #define VULKAN_DLL_API
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

VULKAN_DLL_API const char *get_hello();

#ifdef __cplusplus
}
#endif

#endif // LIB_H
