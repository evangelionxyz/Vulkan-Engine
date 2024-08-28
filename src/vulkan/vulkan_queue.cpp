// Copyright (c) 2024, Evangelion Manuhutu
#include "vulkan_queue.h"
#include "vulkan_wrapper.h"

VulkanQueue::VulkanQueue(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks *allocator,  u32 queue_family_index, u32 queue_index)
    : m_Device(device), m_Swapchain(swapchain), m_Allocator(allocator)
{
    vkGetDeviceQueue(m_Device, queue_family_index, queue_index, &m_Queue);
    LOG_INFO("[Vulkan] Queue Acquired");
    create_semaphores();
}

u32 VulkanQueue::acquired_next_image() const
{
    u32 image_index = 0;
    const VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain,
        UINT64_MAX, m_PresentCompleteSemaphore, nullptr, &image_index);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to acquired image at index {0}", image_index);
    return image_index;
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

    VkResult result = vkQueueSubmit(m_Queue, 1, &submit_info, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to submit");
}

void VulkanQueue::submit_async(VkCommandBuffer cmd) const
{
    VkPipelineStageFlags wait_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info         = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = VK_NULL_HANDLE;
    submit_info.waitSemaphoreCount   = 1;
    submit_info.pWaitSemaphores      = &m_PresentCompleteSemaphore;
    submit_info.pWaitDstStageMask    = &wait_flags;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = &m_RenderCompleteSemaphore;

    const VkResult result = vkQueueSubmit(m_Queue, 1, &submit_info, nullptr);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to submit");
}

void VulkanQueue::present(const u32 image_index) const
{
    VkPresentInfoKHR present_info   = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = VK_NULL_HANDLE;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &m_RenderCompleteSemaphore;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &m_Swapchain;
    present_info.pImageIndices      = &image_index;

    const VkResult result = vkQueuePresentKHR(m_Queue, &present_info);
    wait_idle();
    VK_ERROR_CHECK(result, "[Vulkan] Failed to present");
}

void VulkanQueue::wait_idle() const
{
    vkQueueWaitIdle(m_Queue);
}

void VulkanQueue::destroy_semaphores() const
{
    vkDestroySemaphore(m_Device, m_PresentCompleteSemaphore, m_Allocator);
    vkDestroySemaphore(m_Device, m_RenderCompleteSemaphore, m_Allocator);
}

VkQueue VulkanQueue::get_queue() const
{
    return m_Queue;
}

void VulkanQueue::create_semaphores()
{
    m_PresentCompleteSemaphore = vk_create_semaphore(m_Device, m_Allocator);
    m_RenderCompleteSemaphore = vk_create_semaphore(m_Device, m_Allocator);
}
