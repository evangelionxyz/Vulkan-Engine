// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_physical_device.hpp"
#include "vulkan_wrapper.hpp"
#include "core/assert.hpp"

#include <cstdio>
#include <vulkan/vulkan.h>

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
    : m_Surface(surface)
{
    u32 device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to enumerate devices. Device count: {}", device_count);
    Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Found {} Physical Devices", device_count);

    m_Devices.resize(device_count);
    std::vector<VkPhysicalDevice> temp_devices(device_count);

    vkEnumeratePhysicalDevices(instance, &device_count, temp_devices.data());
    for (u32 device_index = 0; device_index < device_count; device_index++)
    {
        VkPhysicalDevice physical_device = temp_devices[device_index];
        m_Devices[device_index].device = physical_device;
        PhysicalDevice &current_device = m_Devices[device_index];

        vkGetPhysicalDeviceProperties(physical_device, &current_device.properties);
        Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Device name: {}", current_device.properties.deviceName);
        u32 apiVersion = current_device.properties.apiVersion;
        Logger::get_instance().push_message(LoggingLevel::Info, "\t[Vulkan] API Version {}.{}.{}.{}",
            VK_API_VERSION_VARIANT(apiVersion),
            VK_API_VERSION_MAJOR(apiVersion),
            VK_API_VERSION_MINOR(apiVersion),
            VK_API_VERSION_PATCH(apiVersion));

        u32 queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
        Logger::get_instance().push_message(LoggingLevel::Info, "\t[Vulkan] Queue Family Count: {}", queue_families_count);

        current_device.queue_family_properties.resize(queue_families_count);
        current_device.queue_support_present.resize(queue_families_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count,
            current_device.queue_family_properties.data());

        for (u32 queue_index = 0; queue_index < queue_families_count; queue_index++)
        {
            const VkQueueFamilyProperties& queue_family_properties = current_device.queue_family_properties[queue_index];

            Logger::get_instance().push_message(LoggingLevel::Info, "\t[Vulkan] Family {} Num Queues: {}", queue_index, queue_families_count);

            VkQueueFlags flags = queue_family_properties.queueFlags;

            Logger::get_instance().push_message(LoggingLevel::Info,
                "\t[Vulkan] GFX {}, Compute {}, Transfer {}, Sparse binding {}",
                (flags & VK_QUEUE_GRAPHICS_BIT)       ? "Yes" : "No",
                (flags & VK_QUEUE_COMPUTE_BIT)        ? "Yes" : "No",
                (flags & VK_QUEUE_TRANSFER_BIT)       ? "Yes" : "No",
                (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No"
            );
            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, surface,
                &current_device.queue_support_present[queue_index]);
            VK_ERROR_CHECK(result, "[Vulkan] Failed to get physical surface support");
        }

        // get physical surface format
        m_Devices[device_index].surface_formats = get_surface_format(physical_device, surface);

        // get surface capabilities
        current_device.surface_capabilities = get_surface_capabilities(physical_device, m_Surface);

        vk_print_image_usage_flags(current_device.surface_capabilities.supportedUsageFlags);

        // get present modes
        current_device.present_modes = get_surface_present_modes(physical_device, surface);

        // get memory properties and features
        vkGetPhysicalDeviceMemoryProperties(physical_device, &current_device.memory_properties);
        vkGetPhysicalDeviceFeatures(current_device.device, &current_device.features);
    }
}

VkSurfaceCapabilitiesKHR  VulkanPhysicalDevice::get_surface_capabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get surface capabiliteis");
    return capabilities;
}

u32 VulkanPhysicalDevice::select_device(VkQueueFlags required_queue_flags, bool support_present)
{
    for (i32 device_index = 0; device_index < m_Devices.size(); device_index++)
    {
        for (i32 queue_index = 0; queue_index < static_cast<i32>(m_Devices[device_index].queue_family_properties.size()); queue_index++)
        {
            const VkQueueFamilyProperties &queue_properties = m_Devices[device_index].queue_family_properties[queue_index];
            if ((queue_properties.queueFlags & required_queue_flags)
                && (m_Devices[device_index].queue_support_present[queue_index] == static_cast<VkBool32>(support_present)))
            {
                m_DeviceIndex = device_index;
                const i32 queue_family = queue_index;
                Logger::get_instance().push_message(LoggingLevel::Info, "[Vulkan] Using GFX Device {} and queue family {1}", m_DeviceIndex, queue_family);
                return queue_family;
            }
        }
    }
    ASSERT(false, "[Vulkan] Required queue type {} and supports present {1} not found", required_queue_flags, support_present);
    return 0;
}

PhysicalDevice VulkanPhysicalDevice::get_selected_device() const
{
    ASSERT(m_DeviceIndex >= 0, "[Vulkan] A physical device has not been selected");
    return m_Devices[m_DeviceIndex];
}

VkSurfaceFormats VulkanPhysicalDevice::get_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    u32 format_count = 0;
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get physical surface format count");
    ASSERT(format_count, "[Vulkan] Could not get surface format count");
    VkSurfaceFormats surface_formats(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
        surface_formats.data());
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get physical surface format");
    return surface_formats;
}

VkPresentModes VulkanPhysicalDevice::get_surface_present_modes(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    u32 present_mode_count = 0;
    VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get present mode count");
    ASSERT(present_mode_count, "[Vulkan] Could not get physical device surface present mode");

    VkPresentModes present_modes(present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count,
        present_modes.data());
    VK_ERROR_CHECK(result, "[Vulkan] Failed to get physical device surface");
    Logger::get_instance().push_message(LoggingLevel::Info, "\t[Vulkan] Present Modes Count: {}", present_mode_count);

    return present_modes;
}
