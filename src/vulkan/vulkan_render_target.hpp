// Copyright (c) 2025 Evangelion Manuhutu

#ifndef VULKAN_RENDER_TARGET_HPP
#define VULKAN_RENDER_TARGET_HPP

#include <vulkan/vulkan.h>
#include <vector>

#include "core/types.hpp"

struct VulkanRenderTargetAttachment
{
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect;
    VkImageLayout layout;
    VkImageView imageView;
};

struct VulkanRenderTargetInfo
{
    std::vector<VulkanRenderTargetAttachment> attachments;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

class VulkanRenderTarget
{
public:
    VulkanRenderTarget(const VulkanRenderTargetInfo &info, const u32 width, const u32 height);
    ~VulkanRenderTarget();

    VkImageView get_image_view(u32 index) const;
    VkFramebuffer get_framebuffer(u32 index) const;
private:
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;
    VulkanRenderTargetInfo m_Info;
};

#endif