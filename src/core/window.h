// Copyright 2024, Evangelion Manuhutu
#ifndef WINDOW_H
#define WINDOW_H

#include "glfw/include/GLFW/glfw3.h"
#include "types.h"

struct WindowData
{
    i32 Width = 0;
    i32 Height = 0;
    bool Fullscreen = false;
};

class Window {
public:
    Window(i32 width, i32 height, const char *title);
    ~Window();

    [[nodiscard]] bool is_looping() const { return glfwWindowShouldClose(m_Window) == false; }
    void poll_events();
    [[nodiscard]] i32 get_width() const { return m_Data.Width; }
    [[nodiscard]] i32 get_height() const { return m_Data.Height; };
    [[nodiscard]] GLFWwindow *get_native_window() const { return m_Window;};

private:
    void setup_callbacks();
    GLFWwindow *m_Window;
    WindowData m_Data{};
};

#endif //WINDOW_H
