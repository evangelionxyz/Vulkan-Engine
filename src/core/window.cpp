// Copyright 2024, Evangelion Manuhutu
#include "window.h"
#include "assert.h"

Window::Window(const i32 width, const i32 height, const char* title)
{
    if (const i32 success = glfwInit(); !success)
    {
        LOG_ERROR("[Window] Could not initialize GLFW");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    m_Data.Width = width;
    m_Data.Height = height;

    glfwSetWindowUserPointer(m_Window, &m_Data);
    setup_callbacks();

    LOG_INFO("[Window] Window created");
}

Window::~Window()
{
    glfwDestroyWindow(m_Window);
    glfwTerminate();
    LOG_INFO("[Window] Window destroyed");
}

void Window::poll_events()
{
    glfwPollEvents();
}

void Window::setup_callbacks()
{
    glfwSetErrorCallback([](i32 error_code, const char* description)
    {
        ASSERT(false, "GLFW Error: '{0}' :{1}", description, error_code);
    });

    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, i32 width, i32 height)
    {
        WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data.Width = width;
        data.Height = height;
    });
}
