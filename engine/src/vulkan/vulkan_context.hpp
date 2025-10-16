// Copyright (c) 2025, Evangelion Manuhutu

#ifndef VULKAN_CONTEXT_HPP
#define VULKAN_CONTEXT_HPP

#include <unordered_map>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include "vulkan_queue.hpp"
#include "vulkan_swapchain.hpp"
#include "physical_device.hpp"

#include <glm/glm.hpp>

class Window;
class VulkanContext {
public:
    explicit VulkanContext(Window *window);
    void destroy();

    void create_framebuffers();
    void destroy_framebuffers();
    void reset_command_pool() const;

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

    void should_recreate_swapchain();
    
    std::optional<uint32_t> begin_frame();
    void present();

    VkFramebuffer get_framebuffer(uint32_t image_index) const;

    void create_instance();
    void create_debug_callback();
    void create_window_surface();
    void create_device();

    void create_swapchain();

    void create_command_pool();
    void create_descriptor_pool();
    void create_render_pass();

    VkResult reset_command_buffer(VkCommandBuffer command_buffer);
    void submit(const std::vector<VkCommandBuffer> &command_buffers);
    uint32_t get_current_image_index();
private:
    void recreate_swap_chain();

    Window* m_Window                   = nullptr;
    VkInstance m_Instance              = VK_NULL_HANDLE;
    VkDevice m_Device                  = VK_NULL_HANDLE;

    VkSurfaceKHR m_Surface             = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool        = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool  = VK_NULL_HANDLE;
    VkRenderPass m_RenderPass          = VK_NULL_HANDLE;

    VulkanPhysicalDevice m_PhysicalDevice;
    VulkanSwapchain m_SwapChain;
    VulkanQueue m_Queue;
    uint32_t m_QueueFamily               = 0;
    uint32_t m_ImageIndex                = 0;

    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_Framebuffers;

    bool m_ShouldRecreatingSwapChain = false;
};

#endif //VULKAN_CONTEXT_HPP
