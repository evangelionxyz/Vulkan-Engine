// Copyright (c) 2024, Evangelion Manuhutu

#ifndef VULKAN_PHYSICAL_DEVICE_HPP
#define VULKAN_PHYSICAL_DEVICE_HPP

#include "core/types.hpp"
#include <vulkan/vulkan.h>
#include <vector>

struct PhysicalDevice
{
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
    std::vector<VkBool32> queue_support_present;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> present_modes;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceFeatures fetures;
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
