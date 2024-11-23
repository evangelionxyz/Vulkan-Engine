// Copyright 2024, Evangelion Manuhutu

#include "vulkan_buffer.hpp"
#include "vulkan_wrapper.hpp"

VulkanVertexBuffer::VulkanVertexBuffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize buffer_size, VkAllocationCallbacks *allocator)
    : m_Device(device), m_PhysicalDevice(physical_device), m_BufferSize(buffer_size), m_Allocator(allocator)
{
    VkBufferCreateInfo create_info = {};
    create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size        = buffer_size;
    create_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(m_Device, &create_info, allocator, &m_VertexBuffer);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create buffer");

    // memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(m_Device, m_VertexBuffer, &mem_requirements);

    // allocate memory
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(
        m_PhysicalDevice,
        mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    result = vkAllocateMemory(m_Device, &alloc_info, m_Allocator, &m_VertexBufferMemory);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to allocate vertex buffer memory");

    vkBindBufferMemory(m_Device, m_VertexBuffer, m_VertexBufferMemory, 0);
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
}

void VulkanVertexBuffer::destroy()
{
    vkDestroyBuffer(m_Device, m_VertexBuffer, m_Allocator);
    vkFreeMemory(m_Device, m_VertexBufferMemory, m_Allocator);
}
