cmake_minimum_required(VERSION 3.29)
project(Vulkan)

set(CMAKE_CXX_STANDARD 23)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TP_DIR ${ROOT_DIR}/third_party)

include(third_party/cmakefiles/glfw.cmake)
include(third_party/cmakefiles/spdlog.cmake)

add_executable(Vulkan
    src/main.cpp
    src/core/window.cpp
    src/core/window.h
    src/vulkan/vulkan_context.cpp
    src/vulkan/vulkan_context.h
    src/core/assert.h
    src/vulkan/vulkan_physical_device.cpp
    src/vulkan/vulkan_physical_device.h
    src/vulkan/vulkan_wrapper.h
    src/core/types.h
    src/vulkan/vulkan_queue.cpp
    src/vulkan/vulkan_queue.h
    src/core/logger.cpp
    src/core/logger.h
)

add_dependencies(Vulkan GLFW SPDLOG)
target_link_libraries(Vulkan PRIVATE GLFW SPDLOG)

target_include_directories(Vulkan PRIVATE
    src
    ${TP_DIR}/glfw/include
    ${TP_DIR}/spdlog/include
    ${TP_DIR}
)

target_compile_definitions(Vulkan PRIVATE GLFW_INCLUDE_NONE)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Vulkan PRIVATE VK_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Vulkan PRIVATE VK_RELEASE)
endif()

if (WIN32)
    target_compile_definitions(Vulkan PRIVATE PLATFORM_WINDOWS)
elseif (LINUX)
    target_compile_definitions(Vulkan PRIVATE PLATFORM_LINUX)
    target_include_directories(Vulkan PRIVATE /usr/include)
    target_link_directories(Vulkan PRIVATE /usr/lib)
    target_link_libraries(Vulkan PRIVATE
        vulkan shaderc_shared spirv-cross-core spirv-cross-glsl
        pthread dl m rt
    )
endif()

