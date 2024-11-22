// Copyright (c) 2024, Evangelion Manuhutu

#include "vulkan_queue.hpp"
#include "vulkan_wrapper.hpp"

VulkanQueue::VulkanQueue(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks *allocator,  u32 queue_family_index, u32 queue_index)
    : m_Device(device), m_Allocator(allocator)
{
    // create queue
    vkGetDeviceQueue(m_Device, queue_family_index, queue_index, &m_Queue);
    Logger::get_instance().push_message("[Vulkan] Queue Acquired");
    create_semaphores();
}

void VulkanQueue::submit_sync(VkCommandBuffer cmd) const
{
    VkSubmitInfo submit_info = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = VK_NULL_HANDLE;
    submit_info.waitSemaphoreCount   = 0;
    submit_info.pWaitSemaphores      = VK_NULL_HANDLE;
    submit_info.pWaitDstStageMask    = VK_NULL_HANDLE;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores    = VK_NULL_HANDLE;

    VK_ERROR_CHECK(vkQueueSubmit(m_Queue, 1, &submit_info, m_InFlightFence),
        "[Vulkan] Failed to submit");
}

void VulkanQueue::submit_async(const VkCommandBuffer cmd) const
{
    // submit command buffer
    VkSemaphore wait_semaphores[]      = {m_ImageAvailableSemaphore};
    VkSemaphore signaled_semaphores[]  = {m_RenderFinishedSemaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitDstStageMask    = wait_stages;
    submit_info.commandBufferCount   = 1u;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 1u;
    submit_info.pSignalSemaphores    = signaled_semaphores;
    submit_info.waitSemaphoreCount   = 1u;
    submit_info.pWaitSemaphores      = wait_semaphores;

    vkQueueSubmit(m_Queue, 1u, &submit_info, m_InFlightFence);
}

VkResult VulkanQueue::present(const u32 image_index, VkSwapchainKHR swapchain) const
{
    VkSemaphore signaled_semaphores[] = {m_RenderFinishedSemaphore};
    VkPresentInfoKHR present_info = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1u;
    present_info.pWaitSemaphores    = signaled_semaphores;
    present_info.swapchainCount     = 1u;
    present_info.pSwapchains        = &swapchain;
    present_info.pImageIndices      = &image_index;
    present_info.pResults           = VK_NULL_HANDLE;

    return vkQueuePresentKHR(m_Queue, &present_info);
}

void VulkanQueue::wait_idle() const
{
    vkQueueWaitIdle(m_Queue);
}

void VulkanQueue::destroy() const
{
    vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, m_Allocator);
    vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, m_Allocator);
    vkDestroyFence(m_Device, m_InFlightFence, m_Allocator);
}

void VulkanQueue::wait_and_reset_fences() const
{
    // wait for the previous frame to finish
    vkWaitForFences(m_Device, 1u, &m_InFlightFence, VK_TRUE, UINT64_MAX);
    // reset the fence to be signaled again later
    vkResetFences(m_Device, 1, &m_InFlightFence);
}

VkQueue VulkanQueue::get_vk_queue() const
{
    return m_Queue;
}

void VulkanQueue::create_semaphores()
{
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(m_Device, &fence_info, nullptr, &m_InFlightFence);
    vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &m_ImageAvailableSemaphore);
    vkCreateSemaphore(m_Device, &semaphore_info, nullptr, &m_RenderFinishedSemaphore);
}
