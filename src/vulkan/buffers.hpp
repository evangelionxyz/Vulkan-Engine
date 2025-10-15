// Copyright (c) 2025 Evangelion Manuhutu

#ifndef VULKAN_BUFFER_HPP
#define VULKAN_BUFFER_HPP

#include <cstring>
#include <vulkan/vulkan.h>
#include "renderer/vertex.hpp"

#include "vulkan_wrapper.hpp"

static u32 find_memory_type(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_prop;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_prop);

    // Iterate over available memory types, not heaps
    for (u32 i = 0; i < mem_prop.memoryTypeCount; ++i)
    {
        if ((type_filter & (1u << i)) && (mem_prop.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
        }
    }
    throw std::runtime_error("[Vulkan] Failed to find suitable memory type!");
}

static void copy_data_to_buffer(VkDevice device, VkDeviceMemory buffer_memory, 
    void *data, VkDeviceSize size)
{
    void *mapped_data;
    vkMapMemory(device, buffer_memory, 0, size, 0, &mapped_data);
    std::memcpy(mapped_data, data, size);
    vkUnmapMemory(device, buffer_memory);
}

class VertexBuffer
{
public:
    VertexBuffer() = default;

    VertexBuffer(void *data, VkDeviceSize size);
    ~VertexBuffer();

    void set_data(void *data, VkDeviceSize size);

    Ref<VertexBuffer> create(void *data, VkDeviceSize size);
    
    VkDeviceMemory get_buffer_memory() const { return m_BufferMemory; }
    VkBuffer get_buffer() const { return m_Buffer; }

    void destroy();

private:
    VkDeviceSize m_BufferSize;
    VkBuffer m_Buffer;
    VkDeviceMemory m_BufferMemory;
};

#endif