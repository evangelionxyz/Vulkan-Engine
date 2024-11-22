// Copyright (c) 2024, Evangelion Manuhutu
#include "vulkan_swapchain.h"
#include "vulkan_wrapper.h"

VulkanSwapchain::VulkanSwapchain(VkDevice device, VkAllocationCallbacks *allocator, VkSurfaceKHR surface,
    VkSurfaceFormatKHR surface_format, VkSurfaceCapabilitiesKHR capabilities,
    VkPresentModeKHR present_mode, VkImageUsageFlags image_usage_flags, u32 queue_family_index)
    : m_Device(device), m_Format(surface_format)
{
    m_MinImageCount = vk_choose_images_count(capabilities);

    m_ImageExtent = capabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface               = surface;
    swapchain_create_info.minImageCount         = m_MinImageCount;
    swapchain_create_info.imageFormat           = surface_format.format;
    swapchain_create_info.imageColorSpace       = surface_format.colorSpace;
    swapchain_create_info.imageExtent           = m_ImageExtent;
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

    VkResult result = vkCreateSwapchainKHR(device, &swapchain_create_info, allocator, &m_Swapchain);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create swapchain");
    Logger::get_instance().push_message("[Vulkan] Swapchain created");

    // create swapchain images
    u32 swapchain_image_count = 0;
    result = vkGetSwapchainImagesKHR(device, m_Swapchain, &swapchain_image_count, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get swapchain count");
    ASSERT(m_MinImageCount <= swapchain_image_count, "[Vulkan] Swapchain image count exceeds maximum number of images");
    Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Requested {0} images, created {1} images", swapchain_image_count, swapchain_image_count);

    create_image_views(device, allocator, swapchain_image_count);
}

VkResult VulkanSwapchain::acquire_next_image(u32 *image_index, VkSemaphore semaphore)
{
    return vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, image_index);
}

void VulkanSwapchain::create_image_views(VkDevice device, VkAllocationCallbacks* allocator, u32 image_count)
{
    m_Images.resize(image_count);
    m_ImageViews.resize(image_count);

    const VkResult result = vkGetSwapchainImagesKHR(device, m_Swapchain, &image_count, m_Images.data());
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get swapchain images");

    for (u32 i = 0; i < image_count; ++i)
    {
        constexpr i32 mip_levels = 1;
        constexpr i32 layer_count = 1;
        m_ImageViews[i] = vk_create_image_view(
            device, m_Images[i], allocator,
            m_Format.format, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_VIEW_TYPE_2D, layer_count, mip_levels
        );
    }
}

void VulkanSwapchain::destroy(const VkAllocationCallbacks* allocator)
{
    // destroy image view
    for (const auto image_view : m_ImageViews)
        vkDestroyImageView(m_Device, image_view, allocator);
    Logger::get_instance().push_message("[Vulkan] Image views destroyed");

    // destroy swapchain
    vkDestroySwapchainKHR(m_Device, m_Swapchain, allocator);
    Logger::get_instance().push_message("[Vulkan] Swapchain destroyed");
}

VkExtent2D VulkanSwapchain::get_vk_extent() const
{
    return m_ImageExtent;
}

VkSwapchainKHR VulkanSwapchain::get_vk_swapchain()
{
    return m_Swapchain;
}

VulkanSwapchain::VkImages VulkanSwapchain::get_vk_images() const
{
    return m_Images;
}

VkImage& VulkanSwapchain::get_vk_image(u32 index)
{
    return m_Images[index];
}

VulkanSwapchain::VkImageViews VulkanSwapchain::get_vk_image_views() const
{
    return m_ImageViews;
}

const VkImageView& VulkanSwapchain::get_vk_image_view(u32 index) const
{
    return m_ImageViews[index];
}

VkSurfaceFormatKHR VulkanSwapchain::get_vk_format() const
{
    return m_Format;
}

u32 VulkanSwapchain::get_vk_image_count() const
{
    return static_cast<u32>(m_Images.size());
}

u32 VulkanSwapchain::get_vk_min_image_count() const
{
    return m_MinImageCount;
}


