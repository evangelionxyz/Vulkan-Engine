// Copyright (c) 2025 Evangelion Manuhutu

#ifndef VULKAN_WRAPPER_HPP
#define VULKAN_WRAPPER_HPP

#include <vulkan/vulkan.h>
#include <vector>

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/types.hpp"

template<typename... Args>
void vk_error_check(VkResult result, Args&&... args)
{
    if (result != VK_SUCCESS)
    {
        Logger::get_instance().push_message(LoggingLevel::Error, "[Vulkan] Assertion failed at {}: line {}", __FILE__, __LINE__);
        Logger::get_instance().push_message(LoggingLevel::Error, std::forward<Args>(args)...);
        DEBUG_BREAK();
    }
}

#define VK_ERROR_CHECK(result, ...)\
{\
    vk_error_check(result, __VA_ARGS__);\
}

static VkBool32 vk_debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData)
{
    // Determine severity level
    LoggingLevel log_level = LoggingLevel::Info;
    const char* severity_str = "INFO";
    
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        log_level = LoggingLevel::Error;
        severity_str = "ERROR";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        log_level = LoggingLevel::Warning;
        severity_str = "WARNING";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        log_level = LoggingLevel::Info;
        severity_str = "INFO";
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        log_level = LoggingLevel::Info;
        severity_str = "VERBOSE";
    }

    // Determine message type
    const char* type_str = "GENERAL";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        type_str = "VALIDATION";
    }
    else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        type_str = "PERFORMANCE";
    }

    // Log the main message with clear formatting
    Logger::get_instance().push_message(log_level, 
        "[Vulkan {}] [{}] {}", 
        severity_str, 
        type_str, 
        pCallbackData->pMessage);

    // Log additional details if available
    if (pCallbackData->pMessageIdName != nullptr) {
        Logger::get_instance().push_message(log_level, 
            "  └─ Message ID: {} ({})", 
            pCallbackData->pMessageIdName, 
            pCallbackData->messageIdNumber);
    }

    // Log object information if available
    if (pCallbackData->objectCount > 0) {
        Logger::get_instance().push_message(log_level, "  └─ Related Objects:");
        for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
            const auto& obj = pCallbackData->pObjects[i];
            const char* obj_name = obj.pObjectName ? obj.pObjectName : "unnamed";
            Logger::get_instance().push_message(log_level, 
                "     • Object[{}]: Type={}, Handle={}, Name={}", 
                i, 
                static_cast<uint32_t>(obj.objectType),
                obj.objectHandle,
                obj_name);
        }
    }

    // Log queue label information if available
    if (pCallbackData->queueLabelCount > 0) {
        Logger::get_instance().push_message(log_level, "  └─ Queue Labels:");
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; ++i) {
            Logger::get_instance().push_message(log_level, 
                "     • {}", 
                pCallbackData->pQueueLabels[i].pLabelName);
        }
    }

    // Log command buffer label information if available
    if (pCallbackData->cmdBufLabelCount > 0) {
        Logger::get_instance().push_message(log_level, "  └─ Command Buffer Labels:");
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i) {
            Logger::get_instance().push_message(log_level, 
                "     • {}", 
                pCallbackData->pCmdBufLabels[i].pLabelName);
        }
    }

    // Return VK_FALSE to indicate that the application should not be terminated
    // Returning VK_TRUE would cause the Vulkan call to be aborted with VK_ERROR_VALIDATION_FAILED_EXT
    return VK_FALSE;
}

static VkPresentModeKHR vk_choose_present_mode(const std::vector<VkPresentModeKHR> &present_modes)
{
    for (const auto mode : present_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
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
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
    VK_ERROR_CHECK(result, "[Vulkan] Failed to begin command buffer");
}

static VkSemaphore vk_create_semaphore(VkDevice device, VkAllocationCallbacks *allocator)
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.flags = 0;
    semaphore_info.pNext = VK_NULL_HANDLE;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    VK_ERROR_CHECK(vkCreateSemaphore(device, &semaphore_info, allocator, &semaphore), "[Vulkan] Failed to create semaphore");
    ASSERT(semaphore != VK_NULL_HANDLE, "[Vulkan] Semaphore is null");

    return semaphore;
}

static VkImageView vk_create_image_view(const VkDevice device, const VkImage image, VkAllocationCallbacks *allocator,
    VkFormat format, VkImageAspectFlags aspect_flags, VkImageViewType view_type, u32 layer_count, u32 mip_levels)
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
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create image view");
    return view;
}

static void vk_print_image_usage_flags(const VkImageUsageFlags usage)
{
    if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Sampled is supported");
    }
    else if (usage & VK_IMAGE_USAGE_STORAGE_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Storage is supported");
    }
    else if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Input attachment is supported");
    }
    else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Depth stencil attachment is supported");
    }
    else if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Color attachment is supported");
    }
    else if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Transfer dst is supported");
    }
    else if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Transfer src is supported");
    }
}

static void vk_print_memory_property(VkMemoryPropertyFlags properties)
{
    if (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "DEVICE LOCAL ");
    }
    else if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "HOST VISIBLE ");
    }
    else if (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "HOST COHERENT ");
    }
    else if (properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "HOST CACHED ");
    }
    else if (properties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "LAZILY ALLOCATED ");
    }
    else if (properties & VK_MEMORY_PROPERTY_PROTECTED_BIT)
    {
        Logger::get_instance().push_message(LoggingLevel::Info, "PROTECTED ");
    }
}

#endif //VULKAN_WRAPPER_HPP
