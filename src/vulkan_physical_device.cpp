// Copyright (c) 2024, Evangelion Manuhutu

#include "vulkan_physical_device.h"
#include <cstdio>
#include "vulkan_wrapper.h"
#include <vulkan/vulkan.h>
#include "assert.h"

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance, const VkSurfaceKHR& surface)
{
    u32 device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    vk_error_check(result);

    printf("Found %d Physical Devices\n", device_count);

    m_Devices.resize(device_count);
    std::vector<VkPhysicalDevice> devices(device_count);

    result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    vk_error_check(result);

    for (u32 device_index = 0; device_index < device_count; device_index++)
    {
        VkPhysicalDevice physical_device = devices[device_index];
        m_Devices[device_index].PhysDevice = physical_device;

        vkGetPhysicalDeviceProperties(physical_device, &m_Devices[device_index].Properties);
        printf("Device name: %s\n", m_Devices[device_index].Properties.deviceName);
        u32 apiVersion = m_Devices[device_index].Properties.apiVersion;
        printf("\tAPI Version %d.%d.%d.%d",
            VK_API_VERSION_VARIANT(apiVersion),
            VK_API_VERSION_MAJOR(apiVersion),
            VK_API_VERSION_MINOR(apiVersion),
            VK_API_VERSION_PATCH(apiVersion));

        u32 queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
        printf("\tQueue Family Count: %d\n", queue_families_count);

        m_Devices[device_index].QueueFamilyProperties.resize(queue_families_count);
        m_Devices[device_index].QueueSupportPresent.resize(queue_families_count);

        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count,
            m_Devices[device_index].QueueFamilyProperties.data());

        for (u32 queue_index = 0; queue_index < queue_families_count; queue_index++)
        {
            const VkQueueFamilyProperties& queue_family_properties = m_Devices[device_index].QueueFamilyProperties[queue_index];
            printf("\tFamily %d Num Queues: %d", queue_index, queue_families_count);
            VkQueueFlags flags = queue_family_properties.queueFlags;
            printf("\tGFX %s, Compute %s, Transfer %s, Sparse binding %s\n",
                (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
                (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No"
            );

            result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, surface,
                &m_Devices[device_index].QueueSupportPresent[queue_index]);
            vk_error_check(result);
        }

        // get physical surface format
        u32 format_count = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        vk_error_check(result);
        ASSERT(format_count, "[Vulkan Physical Device] Could not get surface format %d", result);

        m_Devices[device_index].SurfaceFormats.resize(format_count);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
            m_Devices[device_index].SurfaceFormats.data());
        vk_error_check(result);

        for (u32 format_index = 0; format_index < format_count; format_index++)
        {
            const VkSurfaceFormatKHR &surface_format = m_Devices[device_index].SurfaceFormats[format_index];
            printf("\tFormat %x color space %x\n", surface_format.format, surface_format.colorSpace);
        }

        // create surface capabilities
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &m_Devices[device_index].SurfaceCapabilities);
        vk_error_check(result);

        vk_print_image_usage_flags(m_Devices[device_index].SurfaceCapabilities.supportedUsageFlags);

        // create Present Mode
        u32 present_mode_count = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
        vk_error_check(result);
        ASSERT(present_mode_count, "[Vulkan Physical Device] Could not get physical device surface present mode");

        m_Devices[device_index].PresentModes.resize(present_mode_count);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count,
            m_Devices[device_index].PresentModes.data());
        vk_error_check(result);

        printf("\tPresent Modes Count: %d\n", present_mode_count);

        vkGetPhysicalDeviceMemoryProperties(physical_device, &m_Devices[device_index].MemoryProperties);

        printf("Num memory types: %d\n", m_Devices[device_index].MemoryProperties.memoryTypeCount);
        for (u32 memory_index = 0; memory_index < m_Devices[device_index].MemoryProperties.memoryTypeCount; memory_index++)
        {
            printf("%d: flags %x heap %d ", memory_index,
                m_Devices[device_index].MemoryProperties.memoryTypes[memory_index].propertyFlags,
                m_Devices[device_index].MemoryProperties.memoryTypes[memory_index].heapIndex);
            vk_print_memory_property(m_Devices[device_index].MemoryProperties.memoryTypes[memory_index].propertyFlags);
            printf("\n");
        }

        printf("Heap types count: %d\n\n", m_Devices[device_index].MemoryProperties.memoryHeapCount);
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
                printf("Using GFX Device %d and queue family %d\n", m_DeviceIndex, queue_family);
                return queue_family;
            }
        }
    }
    ASSERT(false, "[Vulkan Physical Device] Required queue type %x and supports present %d not found", required_queue_flags, support_present);
    return 0;
}

const PhysicalDevice& VulkanPhysicalDevice::get_selected_device() const
{
    ASSERT(m_DeviceIndex >= 0, "[Vulkan Physical Device] A physical device has not been selected\n");
    return m_Devices[m_DeviceIndex];
}
