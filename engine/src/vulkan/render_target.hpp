// Copyright (c) 2025 Evangelion Manuhutu

#ifndef VULKAN_RENDER_TARGET_HPP
#define VULKAN_RENDER_TARGET_HPP

#include <vulkan/vulkan.h>
#include <vector>

#include "core/types.hpp"

struct RenderTargetAttachment
{
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect;
    VkImageLayout layout;
    VkImageView imageView;
};

struct RenderTargetInfo
{
    std::vector<RenderTargetAttachment> attachments;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};

class RenderTarget
{
public:
    RenderTarget(const RenderTargetInfo &info, const u32 width, const u32 height);
    ~RenderTarget();

    VkImageView get_image_view(u32 index) const;
    VkFramebuffer get_framebuffer(u32 index) const;
private:
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;
    RenderTargetInfo m_Info;
};

#endif