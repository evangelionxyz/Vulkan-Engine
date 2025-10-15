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
        ASSERT(false, "[Window] Could not initialize GLFW");
    }

    m_Window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);


    m_Data.WindowWidth = width;
    m_Data.WindowHeight = height;

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

bool Window::is_looping() const
{
    return m_Looping;
}

void Window::poll_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_WINDOW_RESIZED:
                {
                    m_Data.WindowWidth = event.window.data1;
                    m_Data.WindowHeight = event.window.data2;

                        VulkanContext *vk_context = VulkanContext::get();
                        vk_context->recreate_swap_chain();
                    break;
                }
            case SDL_EVENT_QUIT:
                {
                    m_Looping = false;
                    break;
                }
        }
    }
}

void Window::present(const glm::vec4 &clear_color)
{
    m_Vk->set_clear_color(clear_color);
    m_Vk->present();
}

void Window::submit(std::function<void(VkCommandBuffer command_buffer)> func)
{
    m_CommandFuncs.push(func);
}

std::queue<std::function<void(VkCommandBuffer command_buffer)>> &Window::get_command_queue()
{
    return m_CommandFuncs;
}