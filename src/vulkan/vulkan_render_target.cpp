// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_render_target.hpp"

#include "vulkan_context.hpp"

VulkanRenderTarget::VulkanRenderTarget(const VulkanRenderTargetInfo &info, const u32 width, const u32 height)
    : m_Info(info)
{
}

VulkanRenderTarget::~VulkanRenderTarget()
{
    const VkDevice device = VulkanContext::get()->get_device();
    for (const auto &iv : m_ImageViews)
    {
        if (iv != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, iv, VK_NULL_HANDLE);
        }
    }
    
    for (const auto &fb : m_Framebuffers)
    {
        if (fb != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, fb, VK_NULL_HANDLE);
        }
    }
}

VkImageView VulkanRenderTarget::get_image_view(u32 index) const
{
    return m_ImageViews[index]; 
}

VkFramebuffer VulkanRenderTarget::get_framebuffer(u32 index) const
{
    return m_Framebuffers[index];
}
