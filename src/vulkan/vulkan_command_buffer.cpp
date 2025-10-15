// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_command_buffer.hpp"
#include "vulkan_context.hpp"

CommandBuffer::CommandBuffer()
{
    const auto device = VulkanContext::get()->get_device();
    const auto command_pool = VulkanContext::get()->get_command_pool();

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(device, &alloc_info, &m_Handle);
}

CommandBuffer::~CommandBuffer()
{
    const auto device = VulkanContext::get()->get_device();
    const auto command_pool = VulkanContext::get()->get_command_pool();

    vkFreeCommandBuffers(device, command_pool, 1, &m_Handle);
}
