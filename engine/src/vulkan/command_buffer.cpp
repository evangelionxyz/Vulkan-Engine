// Copyright (c) 2025 Evangelion Manuhutu

#include "command_buffer.hpp"
#include "vulkan_context.hpp"
#include "vulkan_wrapper.hpp"

CommandBuffer::CommandBuffer(uint32_t count)
    : m_ActiveGraphicsPipeline(VK_NULL_HANDLE)
{
    const auto device = VulkanContext::get()->get_device();
    const auto command_pool = VulkanContext::get()->get_command_pool();
    const uint32_t image_count = VulkanContext::get()->get_swap_chain()->get_image_count();

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count == 0 ? image_count : count
    };

    m_Handles.resize(alloc_info.commandBufferCount);

    VkResult result = vkAllocateCommandBuffers(device, &alloc_info, m_Handles.data());
    VK_ERROR_CHECK(result, "[Vulkan] Failed to allocate command buffer");
}

void CommandBuffer::begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBuffer handle = get_active_handle();

    VkResult reset_result = VulkanContext::get()->reset_command_buffer(handle);
    VK_ERROR_CHECK(reset_result, "[Vulkan] Failed to reset command buffer");

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = VK_NULL_HANDLE,
        .flags = flags,
        .pInheritanceInfo = VK_NULL_HANDLE
    };

    VK_ERROR_CHECK(vkBeginCommandBuffer(handle, &begin_info), "Failed to begin command buffer");

    m_ActiveGraphicsPipeline = VK_NULL_HANDLE;
}

void CommandBuffer::end()
{
    if (m_ActiveGraphicsPipeline)
    {
        vkCmdEndRenderPass(get_active_handle());
    }

    VK_ERROR_CHECK(vkEndCommandBuffer(get_active_handle()), "[Vulkan] Failed to end command buffer recording");
}

void CommandBuffer::destroy()
{
    const auto device = VulkanContext::get()->get_device();
    const auto command_pool = VulkanContext::get()->get_command_pool();
    vkFreeCommandBuffers(device, command_pool, static_cast<uint32_t>(m_Handles.size()), m_Handles.data());

    m_Handles.clear();
}

void CommandBuffer::set_graphics_state(const GraphicsState &state)
{
    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = VK_NULL_HANDLE,
        .renderPass = state.render_pass,
        .framebuffer = state.framebuffer,
        .renderArea = state.scissor,
        .clearValueCount = 1,
        .pClearValues = &state.clear_value,
    };

    VkCommandBuffer active_handle = get_active_handle();

    vkCmdBeginRenderPass(active_handle, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(active_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline);
    m_ActiveGraphicsPipeline = state.pipeline;

    vkCmdSetViewport(active_handle, 0, 1, &state.viewport);
    vkCmdSetScissor(active_handle, 0, 1, &state.scissor);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(active_handle, 0, 1, state.vertex_buffers.data(), offsets);

    if (state.index_buffer.buffer)
    {
        vkCmdBindIndexBuffer(active_handle, state.index_buffer.buffer, state.index_buffer.offset,
            state.index_buffer.index_type);
    }

    if (!state.descriptor_sets.empty())
    {
        vkCmdBindDescriptorSets(active_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline_layout,
            0, static_cast<uint32_t>(state.descriptor_sets.size()), state.descriptor_sets.data(),
            0, nullptr);
    }
}

void CommandBuffer::draw(const DrawArguments &args)
{
    vkCmdDraw(get_active_handle(), args.vertex_count, args.instance_count, args.first_vertex, args.first_instance);
}

void CommandBuffer::draw_indexed(const DrawArguments &args)
{
    vkCmdDrawIndexed(get_active_handle(), args.vertex_count, args.instance_count, args.first_vertex,
        args.vertex_offset, args.first_instance);
}

void CommandBuffer::set_push_constants(VkShaderStageFlagBits shader_stage, VkPipelineLayout layout,
    const void *data, uint32_t size, uint32_t offset)
{
    vkCmdPushConstants(get_active_handle(), layout, shader_stage, 0, size, data);
}

Ref<CommandBuffer> CommandBuffer::create(uint32_t count)
{
    return CreateRef<CommandBuffer>(count);
}

VkCommandBuffer CommandBuffer::get_active_handle()
{
    const uint32_t current_image_index = VulkanContext::get()->get_current_image_index();
    return m_Handles[current_image_index];
}

CommandBuffer::~CommandBuffer()
{
    ASSERT(m_Handles.empty(), "Forget to call destroy()");
}
