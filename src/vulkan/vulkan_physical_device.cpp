// Copyright (c) 2024, Evangelion Manuhutu
#include "vulkan_physical_device.h"
#include "vulkan_wrapper.h"
#include "core/assert.h"

#include <cstdio>
#include <vulkan/vulkan.h>

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance, const VkSurfaceKHR& surface)
{
    u32 device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to enumerate devices. Device count: {0}", device_count);
    LOG_INFO("[Vulkan] Found {0} Physical Devices", device_count);

    m_Devices.resize(device_count);
    std::vector<VkPhysicalDevice> temp_devices(device_count);

    vkEnumeratePhysicalDevices(instance, &device_count, temp_devices.data());
    for (u32 device_index = 0; device_index < device_count; device_index++)
    {
        VkPhysicalDevice physical_device = temp_devices[device_index];
        m_Devices[device_index].PhysDevice = physical_device;
        PhysicalDevice &current_device = m_Devices[device_index];

        vkGetPhysicalDeviceProperties(physical_device, &current_device.Properties);
        LOG_INFO("[Vulkan] Device name: {0}", current_device.Properties.deviceName);
        u32 apiVersion = current_device.Properties.apiVersion;
        LOG_INFO("\t[Vulkan] API Version {0}.{1}.{2}.{3}",
            VK_API_VERSION_VARIANT(apiVersion),
            VK_API_VERSION_MAJOR(apiVersion),
            VK_API_VERSION_MINOR(apiVersion),
            VK_API_VERSION_PATCH(apiVersion));

        u32 queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
        LOG_INFO("\t[Vulkan] Queue Family Count: {0}", queue_families_count);

        current_device.QueueFamilyProperties.resize(queue_families_count);
        current_device.QueueSupportPresent.resize(queue_families_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count,
            current_device.QueueFamilyProperties.data());

        for (u32 queue_index = 0; queue_index < queue_families_count; queue_index++)
        {
            const VkQueueFamilyProperties& queue_family_properties = current_device.QueueFamilyProperties[queue_index];
            LOG_INFO("\t[Vulkan] Family {0} Num Queues: {1}", queue_index, queue_families_count);
            VkQueueFlags flags = queue_family_properties.queueFlags;
            LOG_INFO("\t[Vulkan] GFX {0}, Compute {1}, Transfer {2}, Sparse binding {3}",
                (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No"
            );
            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, surface,
                &current_device.QueueSupportPresent[queue_index]);
            VK_ERROR_CHECK(result, "[Vulkan] Failed to get physical surface support");
        }

        // get physical surface format
        m_Devices[device_index].SurfaceFormats = get_surface_format(physical_device, surface);

        // get surface capabilities
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &current_device.SurfaceCapabilities);
        VK_ERROR_CHECK(result, "[Vulkan] Failed to get surface capabilities");

        vk_print_image_usage_flags(current_device.SurfaceCapabilities.supportedUsageFlags);

        // get present modes
        current_device.PresentModes = get_surface_present_modes(physical_device, surface);

        // get memory properties and features
        vkGetPhysicalDeviceMemoryProperties(physical_device, &current_device.MemoryProperties);
        vkGetPhysicalDeviceFeatures(current_device.PhysDevice, &current_device.Features);
    }
}

u32 VulkanPhysicalDevice::select_device(VkQueueFlags required_queue_flags, bool support_present)
{
    for (i32 device_index = 0; device_index < m_Devices.size(); device_index++)
    {
        for (i32 queue_index = 0; queue_index < static_cast<i32>(m_Devices[device_index].QueueFamilyProperties.size()); queue_index++)
        {
            const VkQueueFamilyProperties &queue_properties = m_Devices[device_index].QueueFamilyProperties[queue_index];
            if ((queue_properties.queueFlags & required_queue_flags)
                && (m_Devices[device_index].QueueSupportPresent[queue_index] == support_present))
            {
                m_DeviceIndex = device_index;
                const i32 queue_family = queue_index;
                LOG_INFO("[Vulkan] Using GFX Device {0} and queue family {1}", m_DeviceIndex, queue_family);
                return queue_family;
            }
        }
    }
    ASSERT(false, "[Vulkan] Required queue type {0} and supports present {1} not found", required_queue_flags, support_present);
    return 0;
}

const PhysicalDevice& VulkanPhysicalDevice::get_selected_device() const
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
    LOG_INFO("\t[Vulkan] Present Modes Count: {0}", present_mode_count);

    return present_modes;
}
