// Copyright (c) 2025 Evangelion Manuhutu

#ifndef VULKAN_COMMAND_BUFFER_HPP
#define VULKAN_COMMAND_BUFFER_HPP

#include "graphics_pipeline.hpp"
#include <vulkan/vulkan.h>
#include <vector>

class CommandBuffer
{
public:
    CommandBuffer(uint32_t count = 0);
    ~CommandBuffer();
    
    void begin(VkCommandBufferUsageFlagBits flags);
    void end();

    void destroy();

    void set_graphics_state(const GraphicsState &state);
    void draw(const DrawArguments &args);
    void draw_indexed(const DrawArguments &args);

    static Ref<CommandBuffer> create(uint32_t count = 0);

    const std::vector<VkCommandBuffer> &get_handles() const { return m_Handles; }
    VkCommandBuffer get_handle(uint32_t index) const { return m_Handles[index]; }
    VkCommandBuffer get_active_handle();
private:
    std::vector<VkCommandBuffer> m_Handles;
    bool m_UseGraphicsState;
};

#endif