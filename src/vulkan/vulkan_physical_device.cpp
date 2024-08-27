// Copyright (c) 2024, Evangelion Manuhutu

#include "vulkan_physical_device.h"
#include <cstdio>
#include "vulkan_wrapper.h"
#include <vulkan/vulkan.h>
#include "core/assert.h"

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance, const VkSurfaceKHR& surface)
{
    u32 device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    VK_ERROR_CHECK(result, "[vkEnumeratePhysicalDevices] Failed get devices");
    LOG_INFO("Found {0} Physical Devices", device_count);

    m_Devices.resize(device_count);
    std::vector<VkPhysicalDevice> devices(device_count);

    result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    VK_ERROR_CHECK(result, "[vkEnumeratePhysicalDevices] Failed to enumerate devices");

    for (u32 device_index = 0; device_index < device_count; device_index++)
    {
        VkPhysicalDevice physical_device = devices[device_index];
        m_Devices[device_index].PhysDevice = physical_device;

        vkGetPhysicalDeviceProperties(physical_device, &m_Devices[device_index].Properties);
        LOG_INFO("Device name: {0}", m_Devices[device_index].Properties.deviceName);
        u32 apiVersion = m_Devices[device_index].Properties.apiVersion;
        LOG_INFO("\tAPI Version {0}.{1}.{2}.{3}",
            VK_API_VERSION_VARIANT(apiVersion),
            VK_API_VERSION_MAJOR(apiVersion),
            VK_API_VERSION_MINOR(apiVersion),
            VK_API_VERSION_PATCH(apiVersion));

        u32 queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
        LOG_INFO("\tQueue Family Count: {0}", queue_families_count);

        m_Devices[device_index].QueueFamilyProperties.resize(queue_families_count);
        m_Devices[device_index].QueueSupportPresent.resize(queue_families_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count,
            m_Devices[device_index].QueueFamilyProperties.data());

        for (u32 queue_index = 0; queue_index < queue_families_count; queue_index++)
        {
            const VkQueueFamilyProperties& queue_family_properties = m_Devices[device_index].QueueFamilyProperties[queue_index];
            LOG_INFO("\tFamily {0} Num Queues: {1}", queue_index, queue_families_count);
            VkQueueFlags flags = queue_family_properties.queueFlags;
            LOG_INFO("\tGFX {0}, Compute {1}, Transfer {2}, Sparse binding {3}",
                (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No"
            );

            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, surface,
                &m_Devices[device_index].QueueSupportPresent[queue_index]);
            VK_ERROR_CHECK(result, "[vkGetPhysicalDeviceSurfaceSupportKHR] Failed to get physical surface supoort");
        }

        // get physical surface format
        u32 format_count = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        VK_ERROR_CHECK(result, "[vkGetPhysicalDeviceSurfaceFormatsKHR] Failed to get physical surface format count");
        ASSERT(format_count, "[Vulkan Physical Device] Could not get surface format count");

        m_Devices[device_index].SurfaceFormats.resize(format_count);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
            m_Devices[device_index].SurfaceFormats.data());
        VK_ERROR_CHECK(result, "[vkGetPhysicalDeviceSurfaceFormatsKHR] Failed to get physical surface format");

        // create surface capabilities
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &m_Devices[device_index].SurfaceCapabilities);
        VK_ERROR_CHECK(result, "[vkGetPhysicalDeviceSurfaceCapabilitiesKHR] Failed to get surface capabilities");

        vk_print_image_usage_flags(m_Devices[device_index].SurfaceCapabilities.supportedUsageFlags);

        // create Present Mode
        u32 present_mode_count = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
        VK_ERROR_CHECK(result, "[vkGetPhysicalDeviceSurfacePresentModesKHR] Failed to get present mode count");
        ASSERT(present_mode_count, "[Vulkan Physical Device] Could not get physical device surface present mode");

        m_Devices[device_index].PresentModes.resize(present_mode_count);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count,
            m_Devices[device_index].PresentModes.data());
        VK_ERROR_CHECK(result, "[vlGetPhysicalDeviceSurfacePresentModeKHR] Failed to get physical device surface");

        LOG_INFO("\tPresent Modes Count: {0}", present_mode_count);

        vkGetPhysicalDeviceMemoryProperties(physical_device, &m_Devices[device_index].MemoryProperties);

        LOG_INFO("Num memory types: {0}", m_Devices[device_index].MemoryProperties.memoryTypeCount);
        for (u32 memory_index = 0; memory_index < m_Devices[device_index].MemoryProperties.memoryTypeCount; memory_index++)
        {
            LOG_INFO("{0}: flags {1} heap {2} ", memory_index,
                m_Devices[device_index].MemoryProperties.memoryTypes[memory_index].propertyFlags,
                m_Devices[device_index].MemoryProperties.memoryTypes[memory_index].heapIndex);
            vk_print_memory_property(m_Devices[device_index].MemoryProperties.memoryTypes[memory_index].propertyFlags);
        }

        LOG_INFO("Heap types count: {0}", m_Devices[device_index].MemoryProperties.memoryHeapCount);
        vkGetPhysicalDeviceFeatures(m_Devices[device_index].PhysDevice, &m_Devices[device_index].Features);
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
                LOG_INFO("Using GFX Device {0} and queue family {1}", m_DeviceIndex, queue_family);
                return queue_family;
            }
        }
    }
    ASSERT(false, "[Vulkan Physical Device] Required queue type {0} and supports present {1} not found", required_queue_flags, support_present);
    return 0;
}

const PhysicalDevice& VulkanPhysicalDevice::get_selected_device() const
{
    ASSERT(m_DeviceIndex >= 0, "[Vulkan Physical Device] A physical device has not been selected");
    return m_Devices[m_DeviceIndex];
}
