// Copyright (c) 2024, Evangelion Manuhutu
#include "application.h"

#include "logger.h"
#include "vulkan/vulkan_context.h"
#include "vulkan/vulkan_wrapper.h"

Application::Application(i32 argc, char **argv)
{
    m_Window = CreateScope<Window>(600, 400, "Vulkan");
    m_Vk = CreateScope<VulkanContext>(m_Window->get_native_window());
}

Application::~Application()
{
    m_Vk->get_queue()->wait_idle();
    m_Vk->free_command_buffers(m_CommandBuffers);
    m_Vk->destroy_framebuffers(m_Framebuffers);
    m_Vk->destroy_render_pass(m_RenderPass);
}

void Application::run()
{
    m_RenderPass = m_Vk->create_render_pass();
    m_Framebuffers = m_Vk->create_framebuffers(m_RenderPass);
    create_command_buffers();
    record_command_buffers();

    while (m_Window->is_looping())
    {
        m_Window->poll_events();
        present();
    }
}

void Application::create_command_buffers()
{
    m_CommandBuffers.resize(m_Vk->get_image_count());
    m_Vk->create_command_buffers(m_Vk->get_image_count(), m_CommandBuffers.data());
    LOG_INFO("[Application] Command buffers created");
}

void Application::record_command_buffers() const
{
    VkClearValue clear_value;
    clear_value.color.float32[0] = 1.0f;
    clear_value.color.float32[1] = 0.1f;
    clear_value.color.float32[2] = 0.5f;
    clear_value.color.float32[3] = 1.0f;

    const VkRect2D render_area = { { 0, 0 },{ static_cast<u32>(m_Window->get_width()), static_cast<u32>(m_Window->get_height()) }};
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext           = VK_NULL_HANDLE;
    render_pass_begin_info.renderPass      = m_RenderPass;
    render_pass_begin_info.renderArea      = render_area;
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues    = &clear_value;

    for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
    {
        vk_begin_command_buffer(m_CommandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
        render_pass_begin_info.framebuffer = m_Framebuffers[i];
        vkCmdBeginRenderPass(m_CommandBuffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(m_CommandBuffers[i]);
        VK_ERROR_CHECK(vkEndCommandBuffer(m_CommandBuffers[i]), "[vkEndCommandBuffer] Failed to end command buffer");
    }

    LOG_INFO("[Application] Command buffers recorded");
}

void Application::present() const
{
    const u32 image_index = m_Vk->get_queue()->acquired_next_image();
    m_Vk->get_queue()->submit_async(m_CommandBuffers[image_index]);
    m_Vk->get_queue()->present(image_index);
}
