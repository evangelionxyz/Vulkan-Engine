// Copyright (c) 2024, Evangelion Manuhutu

#ifndef VULKAN_SWAPCHAIN_HPP
#define VULKAN_SWAPCHAIN_HPP

#include <vulkan/vulkan_core.h>
#include <vector>

#include "core/types.hpp"

class VulkanSwapchain {
public:
    using VkImages = std::vector<VkImage>;
    using VkImageViews = std::vector<VkImageView>;

    VulkanSwapchain () = default;
    VulkanSwapchain(VkDevice device, VkAllocationCallbacks *allocator, VkSurfaceKHR surface,
        VkSurfaceFormatKHR surface_format, VkSurfaceCapabilitiesKHR capabilities, VkPresentModeKHR present_mode,
        VkImageUsageFlags image_usage_flags, u32 queue_family_index);
    void destroy(const VkAllocationCallbacks *allocator);

    [[nodiscard]] VkResult acquire_next_image(u32 *image_index, VkSemaphore semaphore);

    void begin_render_pass(VkCommandBuffer command_bufferm, VkRenderPass render_pass, VkFramebuffer framebuffer, VkClearValue clear_color);

    VkSwapchainKHR get_vk_swapchain();
    VkImages get_vk_images() const;
    VkImage &get_vk_image(u32 index);
    VkImageViews get_vk_image_views() const;
    const VkImageView &get_vk_image_view(u32 index) const;
    VkSurfaceFormatKHR get_vk_format() const;
    u32 get_vk_image_count() const;
    u32 get_vk_min_image_count() const;
    VkExtent2D get_vk_extent() const;

private:
    void create_image_views(VkDevice device, VkAllocationCallbacks *allocator, u32 image_count);

    VkSwapchainKHR m_Swapchain;
    VkDevice m_Device;
    VkExtent2D m_ImageExtent;

    VkSurfaceFormatKHR m_Format;
    VkImages m_Images;
    VkImageViews m_ImageViews;
    u32 m_MinImageCount = 0;
};

#endif //VULKAN_SWAPCHAIN_H
