// Copyright (c) 2024, Evangelion Manuhutu
#ifndef VULKAN_QUEUE_H
#define VULKAN_QUEUE_H

#include <vulkan/vulkan.h>
#include "types.h"

class VulkanQueue {
public:
    VulkanQueue() = default;
    explicit VulkanQueue(VkDevice device, VkSwapchainKHR swapchain, VkAllocationCallbacks *allocator, u32 queue_family_index, u32 queue_index);

    u32 acquired_next_image() const;

    void submit_sync(VkCommandBuffer cmd) const;
    void submit_async(VkCommandBuffer cmd) const;
    void present(u32 image_index) const;
    void wait_idle() const;
    void destroy_semaphores() const;

private:
    void create_semaphores();

    VkDevice m_Device                      = VK_NULL_HANDLE;
    VkQueue m_Queue                        = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain             = VK_NULL_HANDLE;
    VkSemaphore m_RenderCompleteSemaphore  = VK_NULL_HANDLE;
    VkSemaphore m_PresentCompleteSemaphore = VK_NULL_HANDLE;
    VkAllocationCallbacks *m_Allocator     = VK_NULL_HANDLE;
};



#endif //VULKAN_QUEUE_H
