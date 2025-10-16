file(GLOB_RECURSE VULKAN_APP_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

add_library(VulkanEngine SHARED ${VULKAN_APP_SRC})

target_include_directories(VulkanEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${TP_DIR}/imgui
    ${TP_DIR}/sdl3/include
    ${TP_DIR}/glm
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(VulkanEngine PRIVATE VULKAN_SHARED_BUILD VK_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(VulkanEngine PRIVATE VULKAN_SHARED_BUILD VK_RELEASE)
endif()

if (WIN32)
    target_compile_definitions(VulkanEngine PRIVATE PLATFORM_WINDOWS)
    target_include_directories(VulkanEngine PRIVATE ${VULKAN_INCLUDE_DIR})
    target_link_directories(VulkanEngine PRIVATE ${VULKAN_LIBRARY_DIR})
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(VulkanEngine PRIVATE PLATFORM_LINUX)
    target_include_directories(VulkanEngine PRIVATE /usr/include)
    target_link_directories(VulkanEngine PRIVATE /usr/lib)
endif()

