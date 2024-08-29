// Copyright (c) 2024, Evangelion Manuhutu
#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_shader.h>

#include "window.h"

class VulkanContext;
class Application {
public:
    Application(i32 argc, char **argv);
    ~Application();

    void run();

private:
    void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index) const;
    void create_command_buffers();

    void present() const;

    std::vector<VkFramebuffer> m_Framebuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    Scope<Window> m_Window;
    Ref<VulkanContext> m_Vk;
    Ref<VulkanShader> m_Shader;
};

#endif //APPLICATION_H
