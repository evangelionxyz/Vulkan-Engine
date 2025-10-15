// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_queue.hpp"

#include "vulkan_context.hpp"
#include "vulkan_wrapper.hpp"

VulkanQueue::VulkanQueue(const u32 queue_family_index, const u32 queue_index)
{
    const VkDevice device = VulkanContext::get()->get_device();

    // create queue
    vkGetDeviceQueue(device, queue_family_index, queue_index, &m_Handle);
    Logger::get_instance().push_message("[Vulkan] Queue Acquired");
    create_semaphores();
}

void VulkanQueue::submit_sync(const std::vector<VkCommandBuffer> &command_buffers) const
{
    VkSubmitInfo submit_info = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = VK_NULL_HANDLE;
    submit_info.waitSemaphoreCount   = 0;
    submit_info.pWaitSemaphores      = VK_NULL_HANDLE;
    submit_info.pWaitDstStageMask    = VK_NULL_HANDLE;
    submit_info.commandBufferCount   = static_cast<uint32_t>(command_buffers.size());
    submit_info.pCommandBuffers      = command_buffers.data();
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores    = VK_NULL_HANDLE;

    VK_ERROR_CHECK(vkQueueSubmit(m_Handle, 1, &submit_info, m_InFlightFence), "[Vulkan] Failed to submit");
}

void VulkanQueue::submit_async(const std::vector<VkCommandBuffer> &command_buffers) const
{
    // submit command buffer
    const VkSemaphore wait_semaphores[]      = {m_ImageAvailableSemaphore};
    const VkSemaphore signaled_semaphores[]  = {m_RenderFinishedSemaphore};
    constexpr VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = static_cast<uint32_t>(command_buffers.size());
    submit_info.pCommandBuffers      = command_buffers.data();
    submit_info.signalSemaphoreCount = 1u;
    submit_info.pSignalSemaphores    = signaled_semaphores;
    submit_info.waitSemaphoreCount   = 1u;
    submit_info.pWaitSemaphores      = wait_semaphores;

    vkQueueSubmit(m_Handle, 1u, &submit_info, m_InFlightFence);
}

VkResult VulkanQueue::present(const u32 image_index, const VkSwapchainKHR swap_chain) const
{
    const VkSemaphore signaled_semaphores[] = {m_RenderFinishedSemaphore};
    VkPresentInfoKHR present_info = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1u;
    present_info.pWaitSemaphores    = signaled_semaphores;
    present_info.swapchainCount     = 1u;
    present_info.pSwapchains        = &swap_chain;
    present_info.pImageIndices      = &image_index;
    present_info.pResults           = VK_NULL_HANDLE;

    return vkQueuePresentKHR(m_Handle, &present_info);
}

void VulkanQueue::wait_idle() const
{
    vkQueueWaitIdle(m_Handle);
}

void VulkanQueue::destroy() const
{
    const auto device = VulkanContext::get()->get_device();
    vkDestroySemaphore(device, m_ImageAvailableSemaphore, VK_NULL_HANDLE);
    vkDestroySemaphore(device, m_RenderFinishedSemaphore, VK_NULL_HANDLE);
    vkDestroyFence(device, m_InFlightFence, VK_NULL_HANDLE);
}

void VulkanQueue::wait_and_reset_fences() const
{
    const VkDevice device = VulkanContext::get()->get_device();

    vkWaitForFences(device, 1u, &m_InFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &m_InFlightFence);
}

VkQueue VulkanQueue::get_handle() const
{
    return m_Handle;
}

void VulkanQueue::create_semaphores()
{
    const VkDevice device = VulkanContext::get()->get_device();

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(device, &fence_info, nullptr, &m_InFlightFence);
    vkCreateSemaphore(device, &semaphore_info, nullptr, &m_ImageAvailableSemaphore);
    vkCreateSemaphore(device, &semaphore_info, nullptr, &m_RenderFinishedSemaphore);
}
