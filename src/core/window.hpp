// Copyright 2024, Evangelion Manuhutu

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "types.hpp"

#include "vulkan/vulkan_context.hpp"

#include <glm/glm.hpp>
#include <atomic>

struct WindowData
{
    i32 FbWidth = 0;
    i32 FbHeight = 0;
    i32 WindowWidth = 0;
    i32 WindowHeight = 0;
    bool Fullscreen = false;
};

struct SDL_Window;
union SDL_Event;

class Window {
public:
    Window(i32 width, i32 height, const char *title);
    ~Window();

    [[nodiscard]] bool is_looping() const;
    void poll_events(SDL_Event *event);

    void set_window_resize_callback(const std::function<void(uint32_t width, uint32_t height)> &func);
    void set_framebuffer_resize_callback(const std::function<void(uint32_t width, uint32_t height)> &func);

    [[nodiscard]] uint32_t get_framebuffer_width() const { return static_cast<u32>(m_Data.FbWidth); }
    [[nodiscard]] uint32_t get_framebuffer_height() const { return static_cast<u32>(m_Data.FbHeight); };
    [[nodiscard]] uint32_t get_window_width() const { return static_cast<u32>(m_Data.WindowWidth); }
    [[nodiscard]] uint32_t get_window_height() const { return static_cast<u32>(m_Data.WindowHeight); };

    [[nodiscard]] SDL_Window *get_native_window() const { return m_Window;};

    VulkanContext *get_vk_context() const { return m_Vk.get(); }

private:
    Scope<VulkanContext> m_Vk;

    std::function<void(uint32_t width, uint32_t height)> m_WindowResizeCallback;
    std::function<void(uint32_t width, uint32_t height)> m_FramebufferResizeCallback;

    SDL_Window *m_Window;
    std::atomic<bool> m_Looping = true;
    WindowData m_Data{};
};

#endif //WINDOW_H
