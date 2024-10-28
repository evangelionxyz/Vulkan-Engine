// Copyright 2024, Evangelion Manuhutu

#include <vulkan/vulkan.h>

class VulkanImage
{
public:
    VulkanImage(VkDevice device, VkSurfaceKHR surface);
private:
    VkImage m_Image;
};