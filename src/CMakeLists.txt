add_executable(Vulkan
    core/window.cpp
    core/application.cpp
    vulkan_engine.cpp
    vulkan/vulkan_context.cpp
    vulkan/vulkan_shader.cpp
    vulkan/vulkan_physical_device.cpp
    vulkan/vulkan_queue.cpp
    vulkan/vulkan_swapchain.cpp
    vulkan/vulkan_graphics_pipeline.cpp
)

target_include_directories(Vulkan PRIVATE
    ${ROOT_DIR}/src

    ${TP_DIR}
    ${TP_DIR}/glm
    ${TP_DIR}/imgui
    ${TP_DIR}/glfw/include
)

add_dependencies(Vulkan GLFW IMGUI)
target_link_libraries(Vulkan PRIVATE GLFW IMGUI)

target_compile_definitions(Vulkan PRIVATE GLFW_INCLUDE_NONE)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Vulkan PRIVATE VK_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(Vulkan PRIVATE VK_RELEASE)
endif()

if (WIN32)
    target_compile_definitions(Vulkan PRIVATE PLATFORM_WINDOWS)

    target_include_directories(Vulkan PRIVATE ${VULKAN_INCLUDE_DIR})
    target_link_directories(Vulkan PRIVATE ${VULKAN_LIBRARY_DIR})

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(Vulkan PRIVATE
            vulkan-1.lib 
            shaderc_sharedd.lib 
            spirv-cross-cored.lib 
            spirv-cross-glsld.lib
            spirv-cross-reflect.lib
            spirv-cross-util.lib
        )
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_libraries(Vulkan PRIVATE
            vulkan-1.lib 
            shaderc_shared.lib 
            spirv-cross-core.lib 
            spirv-cross-glsl.lib
            spirv-cross-reflect.lib
            spirv-cross-util.lib
        )
    endif()
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(Vulkan PRIVATE PLATFORM_LINUX)

    target_include_directories(Vulkan PRIVATE /usr/include)
    target_link_directories(Vulkan PRIVATE /usr/lib)

    target_link_libraries(Vulkan PRIVATE
        vulkan
        shaderc_shared
        spirv-cross-core
        spirv-cross-glsl
        pthread dl m rt
    )
endif()

