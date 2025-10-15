// Copyright 2024, Evangelion Manuhutu

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "types.hpp"

#include "vulkan/vulkan_context.hpp"

#include <glm/glm.hpp>

struct WindowData
{
    i32 FbWidth = 0;
    i32 FbHeight = 0;
    i32 WindowWidth = 0;
    i32 WindowHeight = 0;
    bool Fullscreen = false;
};

struct SDL_Window;
class Window {
public:
    Window(i32 width, i32 height, const char *title);
    ~Window();

    [[nodiscard]] bool is_looping() const;
    void poll_events();

    [[nodiscard]] u32 get_framebuffer_width() const { return static_cast<u32>(m_Data.FbWidth); }
    [[nodiscard]] u32 get_framebuffer_height() const { return static_cast<u32>(m_Data.FbHeight); };
    [[nodiscard]] u32 get_window_width() const { return static_cast<u32>(m_Data.WindowWidth); }
    [[nodiscard]] u32 get_window_height() const { return static_cast<u32>(m_Data.WindowHeight); };

    [[nodiscard]] SDL_Window *get_native_window() const { return m_Window;};

    VulkanContext *get_vk_context() const { return m_Vk.get(); }

private:
    Scope<VulkanContext> m_Vk;

    SDL_Window *m_Window;
    bool m_Looping = true;
    WindowData m_Data{};
};

#endif //WINDOW_H
