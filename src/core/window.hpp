// Copyright 2024, Evangelion Manuhutu

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "types.hpp"

struct WindowData
{
    i32 FbWidth = 0;
    i32 FbHeight = 0;
    i32 WindowWidth = 0;
    i32 WindowHeight = 0;
    bool Fullscreen = false;
};

struct GLFWwindow;
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

    [[nodiscard]] GLFWwindow *get_native_window() const { return m_Window;};

private:
    void setup_callbacks();
    GLFWwindow *m_Window;
    WindowData m_Data{};
};

#endif //WINDOW_H
