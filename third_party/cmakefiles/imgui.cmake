add_library(IMGUI
    ${TP_DIR}/imgui/imgui.cpp
    ${TP_DIR}/imgui/imgui_demo.cpp
    ${TP_DIR}/imgui/imgui_draw.cpp
    ${TP_DIR}/imgui/imgui_tables.cpp
    ${TP_DIR}/imgui/imgui_widgets.cpp

    ${TP_DIR}/imgui/backends/imgui_impl_vulkan.cpp
    ${TP_DIR}/imgui/backends/imgui_impl_vulkan.h
    ${TP_DIR}/imgui/backends/imgui_impl_sdl3.cpp
    ${TP_DIR}/imgui/backends/imgui_impl_sdl3.h
)

target_compile_definitions(IMGUI PRIVATE SDL_ENABLE_OLD_NAMES)

target_include_directories(IMGUI PRIVATE
    ${TP_DIR}/imgui
    ${TP_DIR}/sdl3/include
    ${VULKAN_INCLUDE_DIR}
)