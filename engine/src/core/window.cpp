// Copyright 2024, Evangelion Manuhutu

#include "window.hpp"

#include "assert.hpp"

#include "vulkan/vulkan_context.hpp"

#include <SDL3/SDL.h>

#ifdef _WIN32
    #include <Windows.h>
    #include <dwmapi.h>
    #pragma comment(lib, "Dwmapi.lib")
#endif

Window::Window(const i32 width, const i32 height, const char* title)
{
    Logger::get_instance().push_message("[Window] Creating window");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        ASSERT(false, "[Window] Could not initialize SDL3");
    }

    m_Window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    m_Data.WindowWidth = width;
    m_Data.WindowHeight = height;

    SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    Logger::get_instance().push_message("[Window] Window created");
    m_Vk = CreateScope<VulkanContext>(this);
}

Window::~Window()
{
    m_Vk->destroy();
    SDL_DestroyWindow(m_Window);
    SDL_Quit();
    Logger::get_instance().push_message("[Window] Window destroyed");
}

void Window::set_title(const std::string &title)
{
    SDL_SetWindowTitle(m_Window, title.c_str());
}

bool Window::is_looping() const
{
    return m_Looping;
}

void Window::poll_events(SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
        {
            m_Data.WindowWidth = event->window.data1;
            m_Data.WindowHeight = event->window.data2;

            if (m_WindowResizeCallback)
            {
                m_WindowResizeCallback(m_Data.WindowWidth, m_Data.WindowHeight);
            }
            break;
        }
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            m_Data.FbWidth = event->window.data1;
            m_Data.FbHeight = event->window.data2;
            if (m_FramebufferResizeCallback)
            {
                m_FramebufferResizeCallback(m_Data.FbWidth, m_Data.FbHeight);
            }

            VulkanContext::get()->should_recreate_swapchain();
            break;
        }
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            if (event->window.windowID == SDL_GetWindowID(m_Window))
            {
                m_Looping = false;
            }
            break;
        }
        case SDL_EVENT_QUIT:
        {
            m_Looping = false;
            break;
        }

    }
}

void Window::set_window_resize_callback(const std::function<void(uint32_t width, uint32_t height)> &func)
{
    m_WindowResizeCallback = func;
}

void Window::set_framebuffer_resize_callback(const std::function<void(uint32_t width, uint32_t height)> &func)
{
    m_FramebufferResizeCallback = func;
}
