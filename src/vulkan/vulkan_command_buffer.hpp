// Copyright (c) 2025 Evangelion Manuhutu

#ifndef VULKAN_COMMAND_BUFFER_HPP
#define VULKAN_COMMAND_BUFFER_HPP

#include <vulkan/vulkan.h>

class CommandBuffer
{
public:
    CommandBuffer();
    ~CommandBuffer();

    void begin();
    void end();

    VkCommandBuffer get_handle() const { return m_Handle; }
private:
    VkCommandBuffer m_Handle;
};

#endif