// Copyright (c) 2024, Evangelion Manuhutu
#ifndef VULKAN_QUEUE_H
#define VULKAN_QUEUE_H

#include <vulkan/vulkan.h>
#include "core/types.h"

class VulkanQueue {
public:
    VulkanQueue() = default;
    explicit VulkanQueue(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks *allocator, u32 queue_family_index, u32 queue_index);

    void submit_sync(VkCommandBuffer cmd) const;
    void submit_async(VkCommandBuffer cmd) const;
    VkResult present(u32 image_index, VkSwapchainKHR swapchain) const;
    void wait_idle() const;
    void destroy() const;
    void wait_and_reset_fences() const;

    [[nodiscard]] VkSemaphore get_semaphore() const { return m_ImageAvailableSemaphore; }
    [[nodiscard]] VkQueue get_vk_queue() const;

private:
    void create_semaphores();
    VkDevice m_Device                      = VK_NULL_HANDLE;
    VkQueue m_Queue                        = VK_NULL_HANDLE;

    VkSemaphore m_ImageAvailableSemaphore  = VK_NULL_HANDLE;
    VkSemaphore m_RenderFinishedSemaphore  = VK_NULL_HANDLE;
    VkFence m_InFlightFence                = VK_NULL_HANDLE;
    VkAllocationCallbacks *m_Allocator     = VK_NULL_HANDLE;
    mutable u32 m_ImageIndex               = 0;
};

#endif //VULKAN_QUEUE_H
