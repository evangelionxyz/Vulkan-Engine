// Copyright 2024, Evangelion Manuhutu

#include "window.h"
#include "assert.h"

void error_callback(i32 error_code, const char* description)
{
    ASSERT(false, "GLFW Error: '{0}' :{1}", description, error_code);
}

Window::Window(i32 width, i32 height, const char* title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    glfwSetErrorCallback(error_callback);
    m_Data.Width = width;
    m_Data.Height = height;

    set_callbacks();
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::poll_events()
{
    glfwPollEvents();
}

void Window::set_callbacks()
{
    glfwSetWindowUserPointer(m_Window, &m_Data);

    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
    {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data.Width = width;
        data.Height = height;
    });
}
