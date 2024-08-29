// Copyright (c) 2024, Evangelion Manuhutu
#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_shader.h>

#include "window.h"
#include <glm/glm.hpp>

class VulkanContext;
class Application {
public:
    Application(i32 argc, char **argv);
    ~Application();

    void run();

private:
    void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index) const;
    void create_command_buffers();

    void imgui_init();
    void imgui_begin();
    void imgui_end();
    void imgui_shutdown() const;

    void present() const;

    std::vector<VkFramebuffer> m_Framebuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    Scope<Window> m_Window;
    Ref<VulkanContext> m_Vk;
    Ref<VulkanShader> m_Shader;

    glm::vec3 m_ClearColor = glm::vec3(0.0f);
};

#endif //APPLICATION_H
