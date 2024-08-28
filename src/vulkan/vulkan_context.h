// Copyright 2024, Evangelion Manuhutu

#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H
#include <vulkan/vulkan.h>
#include "vulkan_physical_device.h"
#include "vulkan_queue.h"
#include "vulkan_swapchain.h"
#include "GLFW/glfw3.h"

class VulkanContext {
public:
    explicit VulkanContext(GLFWwindow *window);
    ~VulkanContext();

    VkRenderPass create_render_pass() const;
    std::vector<VkFramebuffer> create_framebuffers(VkRenderPass render_pass, u32 width, u32 height) const;
    void create_command_buffers(u32 count, VkCommandBuffer *command_buffers) const;

    void free_command_buffers(std::vector<VkCommandBuffer> &command_buffers) const;
    void destroy_render_pass(VkRenderPass render_pass) const;
    void destroy_framebuffers(const std::vector<VkFramebuffer> &frame_buffers) const;
    void reset_command_pool() const;

    void recreate_swapchain();

    VkInstance get_vk_instance() const;
    VkDevice get_vk_logical_device() const;
    VkPhysicalDevice get_vk_physical_device() const;
    VkDescriptorPool get_vk_descriptor_pool();
    VkAllocationCallbacks *get_vk_allocator();
    VkCommandPool get_vk_command_pool() const;
    VkPipelineCache get_vk_pipeline_cache() const;
    VkPipelineLayout get_vk_pipeline_layout() const;
    VkPipeline get_vk_pipeline() const;
    u32 get_vk_queue_family() const;

    VulkanQueue *get_queue();
    VulkanSwapchain *get_swapchain();
    bool is_rebuild_swapchain() const;

private:
    void create_instance();
    void create_debug_callback();
    void create_window_surface();
    void create_device();
    void create_swapchain();
    void create_command_pool();
    void create_descriptor_pool();
    void create_graphics_pipeline();

    GLFWwindow* m_Window               = nullptr;
    VkInstance m_Instance              = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice           = VK_NULL_HANDLE;

    VkSurfaceKHR m_Surface             = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool        = VK_NULL_HANDLE;
    VkAllocationCallbacks *m_Allocator = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool  = VK_NULL_HANDLE;
    VkPipelineCache m_PipelineCache    = VK_NULL_HANDLE;
    VkPipeline m_GraphicsPipeline      = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout  = VK_NULL_HANDLE;

    VulkanPhysicalDevice m_PhysicalDevice;
    VulkanSwapchain m_Swapchain;
    VulkanQueue m_Queue;
    uint32_t m_QueueFamily             = 0;

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    bool m_RebuildSwapchain = false;
};

#endif //VULKAN_CONTEXT_H
