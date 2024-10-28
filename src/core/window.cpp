// Copyright 2024, Evangelion Manuhutu
#include "window.h"
#include "assert.h"

#include <GLFW/glfw3.h>

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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);

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
