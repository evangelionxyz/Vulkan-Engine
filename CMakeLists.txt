cmake_minimum_required(VERSION 3.29)
project(Vulkan)

set(CMAKE_CXX_STANDARD 23)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(TP_DIR ${ROOT_DIR}/third_party)

add_subdirectory(${TP_DIR}/glfw)

add_executable(Vulkan
        src/main.cpp
        src/window.cpp
        src/window.h
        src/vulkan_context.cpp
        src/vulkan_context.h
        src/assert.h
        src/vulkan_physical_device.cpp
        src/vulkan_physical_device.h
        src/vulkan_wrapper.h
        src/types.h
        src/vulkan_queue.cpp
        src/vulkan_queue.h
)

target_include_directories(Vulkan PRIVATE
        ${SRC_DIR}
        ${TP_DIR}/glfw/include
        ${TP_DIR}
)

target_compile_definitions(Vulkan PRIVATE
        _DEBUG
        GLFW_INCLUDE_NONE
)

target_link_libraries(Vulkan PRIVATE glfw)

if (WIN32)
elseif (LINUX)
    target_include_directories(Vulkan PRIVATE /usr/include)
    target_link_directories(Vulkan PRIVATE /usr/lib)
    target_link_libraries(Vulkan PRIVATE
            vulkan shaderc_shared spirv-cross-core spirv-cross-glsl
            pthread dl m rt
    )
endif()

