// Copyright (c) 2024, Evangelion Manuhutu

#ifndef VULKAN_PHYSICAL_DEVICE_HPP
#define VULKAN_PHYSICAL_DEVICE_HPP

#include "core/types.hpp"
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

using VkSurfaceFormats = std::vector<VkSurfaceFormatKHR>;
using VkPresentModes = std::vector<VkPresentModeKHR>;

class VulkanPhysicalDevice {
public:
    VulkanPhysicalDevice() = default;
    VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    u32 select_device(VkQueueFlags required_queue_flags, bool support_present);
    PhysicalDevice get_selected_device() const;

    static VkSurfaceCapabilitiesKHR get_surface_capabilities(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
    static VkSurfaceFormats get_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
    static VkPresentModes get_surface_present_modes(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

private:
    VkSurfaceKHR m_Surface;
    std::vector<PhysicalDevice> m_Devices;
    i32 m_DeviceIndex = -1;
};

#endif //VULKAN_PHYSICAL_DEVICE_H
