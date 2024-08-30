add_library(IMGUI
    ${TP_DIR}/imgui/imgui.cpp
    ${TP_DIR}/imgui/imgui_demo.cpp
    ${TP_DIR}/imgui/imgui_draw.cpp
    ${TP_DIR}/imgui/imgui_tables.cpp
    ${TP_DIR}/imgui/imgui_widgets.cpp

    ${TP_DIR}/imgui/backends/imgui_impl_vulkan.cpp
    ${TP_DIR}/imgui/backends/imgui_impl_vulkan.h
    ${TP_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${TP_DIR}/imgui/backends/imgui_impl_glfw.h
)

target_include_directories(IMGUI PRIVATE
    ${TP_DIR}/imgui
    ${TP_DIR}/glfw/include
    ${VULKAN_INCLUDE_DIR}
)