// Copyright (c) 2024, Evangelion Manuhutu
#include "application.h"

#include "logger.h"
#include "vulkan/vulkan_context.h"
#include "vulkan/vulkan_wrapper.h"

Application::Application(i32 argc, char **argv)
{
    m_Window = CreateScope<Window>(600, 400, "Vulkan");
    m_Vk = CreateScope<VulkanContext>(m_Window->get_native_window());

    m_Shader = CreateRef<VulkanShader>("res/shaders/default.vert", "res/shaders/default.frag");
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
    m_Framebuffers = m_Vk->create_framebuffers(m_RenderPass, m_Window->get_width(), m_Window->get_height());
    create_command_buffers();

    while (m_Window->is_looping())
    {
        m_Window->poll_events();
        present();
    }

    // reset command pool
    m_Vk->reset_command_pool();
}

void Application::record_command_buffer(VkCommandBuffer command_buffer, u32 image_index) const
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pNext = VK_NULL_HANDLE;
    begin_info.pInheritanceInfo = VK_NULL_HANDLE;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkClearValue clear_color = {1.0f, 0.0f, 0.5f, 1.0f };
    const VkRect2D render_area = { { 0, 0 },{ static_cast<u32>(m_Window->get_width()), static_cast<u32>(m_Window->get_height()) }};
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext           = VK_NULL_HANDLE;
    render_pass_begin_info.renderPass      = m_RenderPass;
    render_pass_begin_info.renderArea      = render_area;
    render_pass_begin_info.framebuffer     = m_Framebuffers[image_index];
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues    = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(command_buffer);

    vkEndCommandBuffer(command_buffer);
}

void Application::create_command_buffers()
{
    const u32 image_count = m_Vk->get_swapchain()->get_vk_image_count();
    m_CommandBuffers.resize(image_count);
    m_Vk->create_command_buffers(image_count, m_CommandBuffers.data());
}

void Application::present() const
{
    m_Vk->get_queue()->wait_idle();
    const u32 image_index = m_Vk->get_queue()->acquired_next_image();

    // wait for fences
    m_Vk->get_queue()->wait_and_reset_fences();

    // reset command buffer
    vkResetCommandBuffer(m_CommandBuffers[image_index], 0);
    record_command_buffer(m_CommandBuffers[image_index], image_index);

    // submit command buffer
    m_Vk->get_queue()->submit_async(m_CommandBuffers[image_index]);

    // present the image
    m_Vk->get_queue()->present(image_index);
}

