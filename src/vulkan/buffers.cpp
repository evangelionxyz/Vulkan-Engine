// Copyright (c) 2025 Evangelion Manuhutu

#include "buffers.hpp"
#include "vulkan_wrapper.hpp"

#include "vulkan_context.hpp"

VertexBuffer::VertexBuffer(void *data, VkDeviceSize size)
    : m_BufferSize(size)
{
    const VkDevice device = VulkanContext::get()->get_device();
    const VkPhysicalDevice physical_device = VulkanContext::get()->get_physical_device();

    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .size = size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkResult result = vkCreateBuffer(device, &create_info, VK_NULL_HANDLE, &m_Buffer);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create buffer");

    // memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, m_Buffer, &mem_requirements);

    // allocate memory
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(
        physical_device, mem_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    result = vkAllocateMemory(device, &alloc_info, VK_NULL_HANDLE, &m_BufferMemory);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to allocate vertex buffer memory");

    vkBindBufferMemory(device, m_Buffer, m_BufferMemory, 0);

    void *mapped_data;
    vkMapMemory(device, m_BufferMemory, 0, size, 0, &mapped_data);
    std::memcpy(mapped_data, data, size);
    vkUnmapMemory(device, m_BufferMemory);
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::set_data(void *data, VkDeviceSize size)
{
    const VkDevice device = VulkanContext::get()->get_device();
}

Ref<VertexBuffer> VertexBuffer::create(void *data, VkDeviceSize size)
{
    return CreateRef<VertexBuffer>(data, size);
}

void VertexBuffer::destroy()
{
    const VkDevice device = VulkanContext::get()->get_device();

    vkDestroyBuffer(device, m_Buffer, VK_NULL_HANDLE);
    vkFreeMemory(device, m_BufferMemory, VK_NULL_HANDLE);
}
