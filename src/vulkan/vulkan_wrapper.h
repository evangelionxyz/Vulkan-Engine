// Copyright (c) 2024, Evangelion Manuhutu
#ifndef VULKAN_WRAPPER_H
#define VULKAN_WRAPPER_H
#include <vulkan/vulkan.h>
#include <vector>

#include "core/assert.h"
#include "core/logger.h"

#define VK_ERROR_CHECK(result, ...)\
{\
    if (result != VK_SUCCESS){\
        LOG_ERROR("Vulkan Error at {0}: {1}", __FILE__, __LINE__);\
        LOG_ERROR(__VA_ARGS__);\
        DEBUG_BREAK();\
    }\
}

static VkPresentModeKHR vk_choose_present_mode(const std::vector<VkPresentModeKHR> &present_modes)
{
    for (const auto mode : present_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
    }
    return present_modes[0];
}

static u32 vk_choose_images_count(const VkSurfaceCapabilitiesKHR &capabilities)
{
    const u32 requested_image_count = capabilities.minImageCount + 1;
    u32 final_image_count = 0;
    if (capabilities.maxImageCount > 0 && requested_image_count > capabilities.maxImageCount)
        final_image_count = capabilities.maxImageCount;
    else
        final_image_count = requested_image_count;
    return final_image_count;
}

static VkSurfaceFormatKHR vk_choose_surface_format(const std::vector<VkSurfaceFormatKHR> &formats)
{
    for (const auto format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return formats[0];
}

static void vk_begin_command_buffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags usage_flags)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags            = usage_flags;
    begin_info.pNext            = VK_NULL_HANDLE;
    begin_info.pInheritanceInfo = VK_NULL_HANDLE;

    VkResult result = vkBeginCommandBuffer(cmd, &begin_info);
    VK_ERROR_CHECK(result, "[vkBeginCommandBuffer] Failed to begin command buffer");
}

static VkSemaphore vk_create_semaphore(VkDevice device, VkAllocationCallbacks *allocator)
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.flags = 0;
    semaphore_info.pNext = VK_NULL_HANDLE;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkResult result = vkCreateSemaphore(device, &semaphore_info, allocator, &semaphore);
    VK_ERROR_CHECK(result, "[vkCreateSemaphore] Failed to create semaphore");
    ASSERT(semaphore != VK_NULL_HANDLE, "[vkCreateSemaphore] Semaphore is null");

    return semaphore;
}

static VkImageView vk_create_image_view(const VkDevice device, const VkImage image, VkAllocationCallbacks *allocator, VkFormat format, VkImageAspectFlags aspect_flags,
    VkImageViewType view_type, u32 layer_count, u32 mip_levels)
{
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.flags = 0;
    view_info.image = image;
    view_info.viewType = view_type;
    view_info.format = format;
    view_info.components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    },
    view_info.subresourceRange = {
        .aspectMask = aspect_flags,
        .baseMipLevel = 0,
        .levelCount = mip_levels,
        .baseArrayLayer = 0,
        .layerCount = layer_count,
    };

    VkImageView view = VK_NULL_HANDLE;
    const VkResult result = vkCreateImageView(device, &view_info, allocator, &view);
    VK_ERROR_CHECK(result, "[vkCreateImageView] Failed to create image view");
    return view;
}

static const char *vk_get_debug_severity_str(const VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "Info";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "Error";
    default: return "Unknown";
    }
}

static const char *vk_get_debug_type(const VkDebugUtilsMessageTypeFlagsEXT flags)
{
    switch (flags)
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
    default: return "Unknown";
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data)
{
    LOG_INFO("Debug callback: {}", p_callback_data->pMessage);
    LOG_INFO("\tSeverity {}", vk_get_debug_severity_str(message_severity));
    LOG_INFO("\tType {}", vk_get_debug_type(message_severity));
    LOG_INFO("\tObjects ");
    for (u32 i = 0; i < p_callback_data->objectCount; i++)
    {
        LOG_INFO("%lu ", p_callback_data->pObjects[i].objectHandle);
    }

    return VK_FALSE;
}

static void vk_print_image_usage_flags(const VkImageUsageFlags usage)
{
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        LOG_INFO("Sampled is supported");
    }
    else if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        LOG_INFO("Storage is supported");
    }
    else if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    {
        LOG_INFO("Input attachment is supported");
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        LOG_INFO("Depth stencil attachment is supported");
    }
    else if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        LOG_INFO("Color attachment is supported");
    }
    else if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        LOG_INFO("Transfer dst is supported");
    }
    else if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        LOG_INFO("Transfer src is supported");
    }
}

static void vk_print_memory_property(VkMemoryPropertyFlags properties)
{
    if (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        LOG_INFO("DEVICE LOCAL ");
    }
    else if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        LOG_INFO("HOST VISIBLE ");
    }
    else if (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    {
        LOG_INFO("HOST COHERENT ");
    }
    else if (properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
    {
        LOG_INFO("HOST CACHED ");
    }
    else if (properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
    {
        LOG_INFO("LAZILY ALLOCATED ");
    }
    else if (properties & VK_MEMORY_PROPERTY_PROTECTED_BIT)
    {
        LOG_INFO("PROTECTED ");
    }
}


#endif //VULKAN_WRAPPER_H
