// Copyright (c) 2025, Evangelion Manuhutu

#ifndef VULKAN_CONTEXT_HPP
#define VULKAN_CONTEXT_HPP

#include <unordered_map>

#include "vulkan_queue.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_physical_device.hpp"
#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_buffer.hpp"

#include <glm/glm.hpp>

class Window;
class VulkanContext {
public:
    explicit VulkanContext(Window *window);
    void destroy();

    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_buffers();
    void free_command_buffers() const;
    void destroy_framebuffers() const;
    void reset_command_pool() const;

    void set_clear_color(const glm::vec4 &clear_color);

    VkInstance get_instance() const;
    VkDevice get_device() const;
    VkPhysicalDevice get_physical_device() const;
    VkDescriptorPool get_descriptor_pool() const;
    VkCommandPool get_command_pool() const;
    VkRenderPass get_render_pass() const;
    u32 get_queue_family() const;

    VulkanQueue *get_queue();
    VulkanSwapchain *get_swap_chain();
    static VulkanContext *get();

    void recreate_swap_chain();
    void present();
    void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index);

    void create_instance();
    void create_debug_callback();
    void create_window_surface();
    void create_device();

    void create_swapchain();

    void create_command_pool();
    void create_descriptor_pool();
    void create_render_pass();

private:
    Window* m_Window                   = nullptr;
    VkInstance m_Instance              = VK_NULL_HANDLE;
    VkDevice m_Device                  = VK_NULL_HANDLE;

    VkSurfaceKHR m_Surface             = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool        = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool  = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass          = VK_NULL_HANDLE;

    Ref<VulkanGraphicsPipeline> m_GraphicsPipeline;

    VulkanPhysicalDevice m_PhysicalDevice;
    VulkanSwapchain m_SwapChain;
    VulkanQueue m_Queue;
    uint32_t m_QueueFamily               = 0;
    VkClearValue m_ClearValue{};

    Ref<VulkanVertexBuffer> m_Buffer;

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_MainFrameBuffers;
    std::vector<VkCommandBuffer> m_MainCmdBuffers;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
};

#endif //VULKAN_CONTEXT_HPP
