// Copyright 2024, Evangelion Manuhutu

#ifndef VULKAN_CONTEXT_HPP
#define VULKAN_CONTEXT_HPP

#include <unordered_map>

#include "vulkan_physical_device.hpp"
#include "vulkan_queue.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_graphics_pipeline.hpp"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <functional>

struct GLFWwindow;

class VulkanContext {
public:
    explicit VulkanContext(GLFWwindow *window);
    ~VulkanContext();

    void create_graphics_pipeline();
    void create_framebuffers();
    void create_command_buffers();
    void free_command_buffers();
    void destroy_framebuffers();
    void reset_command_pool();

    void set_clear_color(const glm::vec4 &clear_color);

    VkInstance get_vk_instance() const;
    VkDevice get_vk_logical_device() const;
    VkPhysicalDevice get_vk_physical_device() const;
    VkDescriptorPool get_vk_descriptor_pool();
    VkAllocationCallbacks *get_vk_allocator();
    VkCommandPool get_vk_command_pool() const;
    VkRenderPass get_vk_render_pass() const;
    VkPipelineCache get_vk_pipeline_cache() const;
    u32 get_vk_queue_family() const;

    VulkanQueue *get_queue();
    VulkanSwapchain *get_swapchain();
    static VulkanContext *get_instance();

    void recreate_swapchain();
    void present();
    void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index);

private:
    void create_instance();
    void create_debug_callback();
    void create_window_surface();
    void create_device();
    void create_swapchain();
    void create_command_pool();
    void create_descriptor_pool();
    void create_render_pass();

    GLFWwindow* m_Window               = nullptr;
    VkInstance m_Instance              = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice           = VK_NULL_HANDLE;

    VkSurfaceKHR m_Surface             = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool        = VK_NULL_HANDLE;
    VkAllocationCallbacks *m_Allocator = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool  = VK_NULL_HANDLE;
    VkPipelineCache m_PipelineCache    = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass          = VK_NULL_HANDLE;

    VulkanGraphicsPipeline m_GraphicsPipeline;

    VulkanPhysicalDevice m_PhysicalDevice;
    VulkanSwapchain m_Swapchain;
    VulkanQueue m_Queue;
    u32 m_QueueFamily                  = 0;
    VkClearValue m_ClearValue;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> m_MainFrameBuffers;
    std::vector<VkCommandBuffer> m_MainCmdBuffers;
};

#endif //VULKAN_CONTEXT_H
