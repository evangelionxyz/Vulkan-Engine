// Copyright 2024, Evangelion Manuhutu
#include "window.h"
#include "assert.h"

#include <GLFW/glfw3.h>

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #include <Windows.h>
    #include <dwmapi.h>
    #pragma comment(lib, "Dwmapi.lib")
#endif

Window::Window(const i32 width, const i32 height, const char* title)
{
    i32 success = glfwInit(); 
    if (!success)
    {
        ASSERT(false, "[Window] Could not initialize GLFW");
        exit(EXIT_FAILURE);
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);

#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(m_Window);
    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

    // 7160E8 visual studio purple
    COLORREF rgbRed = 0x00E86071;
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &rgbRed, sizeof(rgbRed));
#endif

    m_Data.WindowWidth = width;
    m_Data.WindowHeight = height;

    glfwGetFramebufferSize(m_Window, &m_Data.FbWidth, &m_Data.FbHeight);

    glfwSetWindowUserPointer(m_Window, &m_Data);
    setup_callbacks();

    Logger::get_instance().push_message("[Window] Window created");
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
    Logger::get_instance().push_message("[Window] Window destroyed");
}

bool Window::is_looping() const
{
    return glfwWindowShouldClose(m_Window) == false;
}

void Window::poll_events()
{
    glfwPollEvents();
}

void Window::setup_callbacks()
{
    glfwSetErrorCallback([](i32 error_code, const char* description)
    {
        ASSERT(false, "GLFW Error: '{}' :{}", description, error_code);
    });

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, i32 width, i32 height)
    {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data.WindowWidth = width;
        data.WindowHeight = height;
    });

    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, i32 width, i32 height)
    {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data.FbWidth = width;
        data.FbHeight = height;
    });
}
