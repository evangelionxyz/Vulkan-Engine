// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_swapchain.hpp"

#include "vulkan_context.hpp"
#include "vulkan_wrapper.hpp"

VulkanSwapchain::VulkanSwapchain(VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkSurfaceCapabilitiesKHR capabilities,
    VkPresentModeKHR present_mode, VkImageUsageFlags image_usage_flags, u32 queue_family_index)
    : m_Format(surface_format)
{
    m_MinImageCount = vk_choose_images_count(capabilities);

    m_Extent = capabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface               = surface;
    swapchain_create_info.minImageCount         = m_MinImageCount;
    swapchain_create_info.imageFormat           = surface_format.format;
    swapchain_create_info.imageColorSpace       = surface_format.colorSpace;
    swapchain_create_info.imageExtent           = m_Extent;
    swapchain_create_info.imageArrayLayers      = 1;
    swapchain_create_info.imageUsage            = image_usage_flags;
    swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices   = &queue_family_index;
    swapchain_create_info.preTransform          = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode           = present_mode;
    swapchain_create_info.clipped               = VK_TRUE;
    swapchain_create_info.oldSwapchain          = VK_NULL_HANDLE;

    const VkDevice device = VulkanContext::get()->get_device();

    VkResult result = vkCreateSwapchainKHR(device, &swapchain_create_info, VK_NULL_HANDLE, &m_Handle);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create swapchain");
    Logger::get_instance().push_message("[Vulkan] Swapchain created");

    // create swapchain images
    u32 swapchain_image_count = 0;
    result = vkGetSwapchainImagesKHR(device, m_Handle, &swapchain_image_count, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get swapchain count");
    ASSERT(m_MinImageCount <= swapchain_image_count, "[Vulkan] Swapchain image count exceeds maximum number of images");
    Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Requested {} images, created {} images", swapchain_image_count, swapchain_image_count);

    create_image_views(swapchain_image_count);
}

VkResult VulkanSwapchain::acquire_next_image(u32 *image_index, VkSemaphore semaphore) const
{
    const VkDevice device = VulkanContext::get()->get_device();
    return vkAcquireNextImageKHR(device, m_Handle, UINT64_MAX, semaphore, VK_NULL_HANDLE, image_index);
}

void VulkanSwapchain::create_image_views(u32 image_count)
{
    m_Images.resize(image_count);
    m_ImageViews.resize(image_count);

    const VkDevice device = VulkanContext::get()->get_device();
    const VkResult result = vkGetSwapchainImagesKHR(device, m_Handle, &image_count, m_Images.data());
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get swapchain images");

    for (u32 i = 0; i < image_count; ++i)
    {
        constexpr i32 mip_levels = 1;
        constexpr i32 layer_count = 1;
        m_ImageViews[i] = vk_create_image_view(
            device, m_Images[i], VK_NULL_HANDLE,
            m_Format.format, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_VIEW_TYPE_2D, layer_count, mip_levels
        );
    }
}

void VulkanSwapchain::destroy()
{
    const VkDevice device = VulkanContext::get()->get_device();
    // destroy image view
    for (const auto image_view : m_ImageViews)
    {
        vkDestroyImageView(device, image_view, VK_NULL_HANDLE);
    }
    Logger::get_instance().push_message("[Vulkan] Image views destroyed");

    // destroy swapchain
    vkDestroySwapchainKHR(device, m_Handle, VK_NULL_HANDLE);
    Logger::get_instance().push_message("[Vulkan] Swapchain destroyed");
}

VkExtent2D VulkanSwapchain::get_extent() const
{
    return m_Extent;
}

VkSwapchainKHR VulkanSwapchain::get_handle()
{
    return m_Handle;
}

VulkanSwapchain::VkImages VulkanSwapchain::get_images() const
{
    return m_Images;
}

VkImage& VulkanSwapchain::get_image(u32 index)
{
    return m_Images[index];
}

VulkanSwapchain::VkImageViews VulkanSwapchain::get_image_views() const
{
    return m_ImageViews;
}

const VkImageView& VulkanSwapchain::get_image_view(u32 index) const
{
    return m_ImageViews[index];
}

VkSurfaceFormatKHR VulkanSwapchain::get_format() const
{
    return m_Format;
}

u32 VulkanSwapchain::get_image_count() const
{
    return static_cast<u32>(m_Images.size());
}

u32 VulkanSwapchain::get_min_image_count() const
{
    return m_MinImageCount;
}


