// Copyright (c) 2024, Evangelion Manuhutu
#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "vulkan/vulkan_shader.hpp"
#include "window.hpp"

#include <glm/glm.hpp>

class VulkanContext;
class Application {
public:
    Application(i32 argc, char **argv);
    ~Application();

    void run();

private:
    void imgui_init();
    void imgui_begin();
    void imgui_end();
    void imgui_shutdown() const;

    Scope<Window> m_Window;
    glm::vec4 m_ClearColor = glm::vec4(0.0f);
};

#endif //APPLICATION_H
