// Copyright 2024, Evangelion Manuhutu

#include "core/window.h"
#include "vulkan/vulkan_context.h"
#include "vulkan/vulkan_wrapper.h"
#include "core/logger.h"

int main()
{
    Logger::init();

    Window window = Window(800, 400, "Vulkan");
    VulkanContext vk = VulkanContext(window.get_native_window());

    VulkanQueue *queue = vk.get_queue();
    u32 image_count = vk.get_image_count();
    VkRenderPass render_pass = vk.create_render_pass();
    std::vector<VkFramebuffer> frame_buffers = vk.create_framebuffers(render_pass);
    std::vector<VkCommandBuffer> command_buffers;

    auto create_frame_buffers = [&]()
    {
        command_buffers.resize(image_count);
        vk.create_command_buffers(image_count, command_buffers.data());
        LOG_INFO("Command buffers created");
    };

    auto record_command_buffers = [&]()
    {
        VkClearValue clear_value;
        clear_value.color.float32[0] = 1.0f;
        clear_value.color.float32[1] = 1.0f;
        clear_value.color.float32[2] = 1.0f;
        clear_value.color.float32[3] = 1.0f;

        VkRect2D render_area = { { 0, 0 }, { static_cast<u32>(window.get_width()), static_cast<u32>(window.get_height()) }};
        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext = nullptr;
        render_pass_begin_info.renderPass = render_pass;
        render_pass_begin_info.renderArea = render_area;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues = &clear_value;

        for (size_t i = 0; i < command_buffers.size(); ++i)
        {
            vk_begin_command_buffer(command_buffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
            render_pass_begin_info.framebuffer = frame_buffers[i];
            vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdEndRenderPass(command_buffers[i]);
            const VkResult result = vkEndCommandBuffer(command_buffers[i]);
            VK_ERROR_CHECK(result, "[vkEndCommandBuffer] Failed to end command buffer");
        }

        LOG_INFO("Command buffers recorded");
    };

    create_frame_buffers();
    record_command_buffers();

    auto render_scene = [&]()
    {
        const u32 image_index = queue->acquired_next_image();
        queue->submit_async(command_buffers[image_index]);
        queue->present(image_index);
    };

    while (window.is_looping())
    {
        window.poll_events();
        render_scene();
    }

    vk.free_command_buffers(static_cast<u32>(command_buffers.size()), command_buffers.data());
    vk.destroy_framebuffers(frame_buffers);
    vkDestroyRenderPass(vk.get_logical_device(), render_pass, vk.get_allocator());
}
