add_library(GLFW
    ${TP_DIR}/glfw/src/context.c
    ${TP_DIR}/glfw/src/init.c
    ${TP_DIR}/glfw/src/input.c
    ${TP_DIR}/glfw/src/monitor.c
    ${TP_DIR}/glfw/src/null_init.c
    ${TP_DIR}/glfw/src/null_joystick.c
    ${TP_DIR}/glfw/src/null_monitor.c
    ${TP_DIR}/glfw/src/null_window.c
    ${TP_DIR}/glfw/src/platform.c
    ${TP_DIR}/glfw/src/vulkan.c
    ${TP_DIR}/glfw/src/window.c
    ${TP_DIR}/glfw/src/osmesa_context.c
    ${TP_DIR}/glfw/src/wgl_context.c
    ${TP_DIR}/glfw/src/egl_context.c
)


if (WIN32)
    target_compile_definitions(GLFW PRIVATE
        _GLFW_WIN32
    )
    target_sources(GLFW PRIVATE
        ${TP_DIR}/glfw/src/win32_init.c
        ${TP_DIR}/glfw/src/win32_joystick.c
        ${TP_DIR}/glfw/src/win32_module.c
        ${TP_DIR}/glfw/src/win32_monitor.c
        ${TP_DIR}/glfw/src/win32_time.c
        ${TP_DIR}/glfw/src/win32_thread.c
        ${TP_DIR}/glfw/src/win32_window.c
    )

elseif (UNIX AND NOT APPLE)
    
    target_compile_definitions(GLFW PRIVATE _GLFW_X11)
    message(">> GLFW: X11")

    target_sources(GLFW PRIVATE
        ${TP_DIR}/glfw/src/posix_time.c
        ${TP_DIR}/glfw/src/posix_thread.c
        ${TP_DIR}/glfw/src/posix_poll.c
        ${TP_DIR}/glfw/src/posix_module.c
        ${TP_DIR}/glfw/src/linux_joystick.c
        ${TP_DIR}/glfw/src/x11_init.c
        ${TP_DIR}/glfw/src/x11_monitor.c
        ${TP_DIR}/glfw/src/x11_window.c
        ${TP_DIR}/glfw/src/xkb_unicode.c
        ${TP_DIR}/glfw/src/glx_context.c
    )
endif()