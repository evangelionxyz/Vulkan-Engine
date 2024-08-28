// Copyright (c) 2024, Evangelion Manuhutu
#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "window.h"

class VulkanContext;
class Application {
public:
    Application(i32 argc, char **argv);
    ~Application();

    void run();

private:
    void create_command_buffers();
    void record_command_buffers() const;
    void present() const;

    std::vector<VkFramebuffer> m_Framebuffers;
    std::vector<VkCommandBuffer> m_CommandBuffers;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;

    Scope<Window> m_Window;
    Ref<VulkanContext> m_Vk;
};

#endif //APPLICATION_H
