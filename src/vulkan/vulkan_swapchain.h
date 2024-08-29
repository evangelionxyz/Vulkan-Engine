// Copyright (c) 2024, Evangelion Manuhutu
#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H
#include <vulkan/vulkan_core.h>
#include <vector>

#include "core/types.h"

class VulkanSwapchain {
public:
    using VkImages = std::vector<VkImage>;
    using VkImageViews = std::vector<VkImageView>;

    VulkanSwapchain () = default;
    VulkanSwapchain(VkDevice device, VkAllocationCallbacks *allocator, VkSurfaceKHR surface,
        VkSurfaceFormatKHR surface_format, VkSurfaceCapabilitiesKHR capabilities, VkPresentModeKHR present_mode,
        VkImageUsageFlags image_usage_flags, u32 queue_family_index);
    void destroy(VkDevice device, const VkAllocationCallbacks *allocator);

    const VkSwapchainKHR* get_vk_swapchain();
    VkImages get_vk_images() const;
    VkImage &get_vk_image(u32 index);
    VkImageViews get_vk_image_views() const;
    const VkImageView &get_vk_image_view(u32 index) const;
    VkSurfaceFormatKHR get_vk_format() const;
    u32 get_vk_image_count() const;
    u32 get_vk_min_image_count() const;

private:
    void create_image_views(VkDevice device, VkAllocationCallbacks *allocator, u32 image_count);

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkSurfaceFormatKHR m_Format;
    VkImages m_Images;
    VkImageViews m_ImageViews;
    u32 m_MinImageCount = 0;
};

#endif //VULKAN_SWAPCHAIN_H
