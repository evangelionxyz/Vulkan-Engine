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

    VkRenderPass create_render_pass();

    std::vector<VkFramebuffer> create_framebuffers(VkRenderPass render_pass);
    void destroy_framebuffers(const std::vector<VkFramebuffer> &framebuffers);

    VkDevice get_logical_device();
    i32 get_image_count() const;
    const VkImage &get_image(i32 index);

    VulkanQueue *get_queue();
    u32 get_queue_family() const;

    void create_command_buffers(u32 count, VkCommandBuffer *command_buffers);
    void free_command_buffers(u32 count, const VkCommandBuffer *command_buffers);

    VkAllocationCallbacks *get_allocator();

private:
    void create_instance();
    void create_debug_callback();
    void create_window_surface();
    void create_device();
    void create_swapchain();
    void create_command_pool();

    GLFWwindow* m_Window               = nullptr;
    VkInstance m_Instance              = VK_NULL_HANDLE;
    VkDevice m_Device                  = VK_NULL_HANDLE;
    VulkanQueue m_Queue;
    uint32_t m_QueueFamily             = 0;
    VkSurfaceKHR m_Surface             = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool        = VK_NULL_HANDLE;
    VkAllocationCallbacks *m_Allocator = VK_NULL_HANDLE;
    VulkanPhysicalDevice m_PhysicalDevice;

    // swapchain
    VkSurfaceFormatKHR m_SwapchainSurfaceFormat{};
    VkSwapchainKHR m_Swapchain                = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
};

#endif //VULKAN_CONTEXT_H
