// Copyright (c) 2025 Evangelion Manuhutu

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
    VulkanSwapchain(VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkSurfaceCapabilitiesKHR capabilities, VkPresentModeKHR present_mode,
        VkImageUsageFlags image_usage_flags, u32 queue_family_index);
    void destroy();

    [[nodiscard]] VkResult acquire_next_image(u32 *image_index, VkSemaphore semaphore) const;

    void begin_render_pass(VkCommandBuffer command_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkClearValue clear_color);

    VkSwapchainKHR get_handle();
    VkImages get_images() const;
    VkImage &get_image(u32 index);
    VkImageViews get_image_views() const;
    const VkImageView &get_image_view(u32 index) const;
    VkSurfaceFormatKHR get_format() const;
    u32 get_image_count() const;
    u32 get_min_image_count() const;
    VkExtent2D get_extent() const;

private:
    void create_image_views(u32 image_count);

    VkSwapchainKHR m_Handle;
    VkExtent2D m_Extent;
    VkSurfaceFormatKHR m_Format;
    VkImages m_Images;
    VkImageViews m_ImageViews;
    u32 m_MinImageCount = 0;
};

#endif //VULKAN_SWAPCHAIN_H
