// Copyright (c) 2024, Evangelion Manuhutu#ifndef VULKAN_PHYSICAL_DEVICE_H
#ifndef VULKAN_PHYSICAL_DEVICE_H
#define VULKAN_PHYSICAL_DEVICE_H

#include "core/types.h"
#include <vulkan/vulkan.h>
#include <vector>

struct PhysicalDevice
{
    VkPhysicalDevice PhysDevice;
    VkPhysicalDeviceProperties Properties;
    std::vector<VkQueueFamilyProperties> QueueFamilyProperties;
    std::vector<VkBool32> QueueSupportPresent;
    std::vector<VkSurfaceFormatKHR> SurfaceFormats;
    std::vector<VkPresentModeKHR> PresentModes;
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    VkPhysicalDeviceFeatures Features;
};

class VulkanPhysicalDevice {
public:
    VulkanPhysicalDevice() = default;
    VulkanPhysicalDevice(VkInstance instance, const VkSurfaceKHR &surface);
    u32 select_device(VkQueueFlags required_queue_flags, bool support_present);
    const PhysicalDevice& get_selected_device() const;

private:
    std::vector<PhysicalDevice> m_Devices;
    i32 m_DeviceIndex = -1;
};

#endif //VULKAN_PHYSICAL_DEVICE_H
