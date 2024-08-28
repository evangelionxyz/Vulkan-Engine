// Copyright 2024, Evangelion Manuhutu

#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H
#include <cstdio>
#include <vulkan/vulkan.h>
#include "vulkan_physical_device.h"
#include "vulkan_queue.h"
#include "GLFW/glfw3.h"

class VulkanContext {
public:
    explicit VulkanContext(GLFWwindow *window);
    ~VulkanContext();

    VkRenderPass create_render_pass() const;

    std::vector<VkFramebuffer> create_framebuffers(VkRenderPass render_pass) const;
    void destroy_framebuffers(const std::vector<VkFramebuffer> &frame_buffers) const;

    VkInstance get_instance() const;
    VkDevice get_logical_device() const;
    VkPhysicalDevice get_physical_device() const;
    VkDescriptorPool get_descriptor_pool();
    VkPipelineCache get_pipeline_cache() const;

    [[nodiscard]] i32 get_image_count() const;
    [[nodiscard]] const VkImage &get_image(i32 index) const;

    VulkanQueue *get_queue();
    [[nodiscard]] u32 get_queue_family() const;

    void create_command_buffers(u32 count, VkCommandBuffer *command_buffers) const;
    void free_command_buffers(u32 count, const VkCommandBuffer *command_buffers);

    VkAllocationCallbacks *get_allocator();

private:
    void create_instance();
    void create_debug_callback();
    void create_window_surface();
    void create_device();
    void create_swapchain();
    void create_command_pool();
    void create_descriptor_pool();

    GLFWwindow* m_Window               = nullptr;
    VkInstance m_Instance              = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice           = VK_NULL_HANDLE;
    VulkanQueue m_Queue;
    uint32_t m_QueueFamily             = 0;
    VkSurfaceKHR m_Surface             = VK_NULL_HANDLE;
    VkSurfaceFormatKHR m_SurfaceFormat;
    VkCommandPool m_CommandPool        = VK_NULL_HANDLE;
    VkAllocationCallbacks *m_Allocator = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool  = VK_NULL_HANDLE;
    VkPipelineCache m_PipelineCache    = VK_NULL_HANDLE;
    VulkanPhysicalDevice m_PhysicalDevice;

    // swapchain
    VkSurfaceFormatKHR m_SwapchainSurfaceFormat{};
    VkSwapchainKHR m_Swapchain                = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
};

#endif //VULKAN_CONTEXT_H
