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
    copy_data_to_buffer(device, m_BufferMemory, data, size);
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::set_data(void *data, VkDeviceSize size)
{
    const VkDevice device = VulkanContext::get()->get_device();
    vkBindBufferMemory(device, m_Buffer, m_BufferMemory, 0);
    copy_data_to_buffer(device, m_BufferMemory, data, size);
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

IndexBuffer::IndexBuffer(const std::vector<uint32_t> &indices)
    : m_Count(static_cast<uint32_t>(indices.size()))
{
    const VkDevice device = VulkanContext::get()->get_device();
    const VkPhysicalDevice physical_device = VulkanContext::get()->get_physical_device();

    VkDeviceSize size = indices.size() * sizeof(uint32_t);
    VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = VK_NULL_HANDLE,
        .size = size,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
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
    copy_data_to_buffer(device, m_BufferMemory, (void *)indices.data(), size);
}

IndexBuffer::~IndexBuffer()
{
}

Ref<IndexBuffer> IndexBuffer::create(const std::vector<uint32_t> &indices)
{
    return CreateRef<IndexBuffer>(indices);
}

void IndexBuffer::destroy()
{
    const VkDevice device = VulkanContext::get()->get_device();
    vkDestroyBuffer(device, m_Buffer, VK_NULL_HANDLE);
    vkFreeMemory(device, m_BufferMemory, VK_NULL_HANDLE);
}
