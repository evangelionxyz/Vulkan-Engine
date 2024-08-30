// Copyright (c) 2024, Evangelion Manuhutu
#include "application.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "logger.h"
#include "vulkan/vulkan_context.h"
#include "vulkan/vulkan_wrapper.h"

Application::Application(i32 argc, char **argv)
{
    m_Window = CreateScope<Window>(1024, 720, "Vulkan Engine");
    m_Vk = CreateScope<VulkanContext>(m_Window->get_native_window());

    m_Shader = CreateRef<VulkanShader>("res/shaders/default.vert", "res/shaders/default.frag");
    m_Vk->create_graphics_pipeline();
}

Application::~Application()
{
    m_Vk->get_queue()->wait_idle();
    m_Vk->free_command_buffers(m_CommandBuffers);
    m_Vk->destroy_framebuffers(m_Framebuffers);
}

void Application::run()
{
    m_Framebuffers = m_Vk->create_framebuffers(m_Window->get_framebuffer_width(), m_Window->get_framebuffer_height());
    create_command_buffers();
    imgui_init();

    while (m_Window->is_looping())
    {
        m_Window->poll_events();
        {
            imgui_begin();
            ImGui::ShowDemoWindow();
            ImGui::Begin("Settings");
            ImGui::ColorEdit3("clear color", &m_ClearColor[0]);
            ImGui::End();
            imgui_end();
        }
        present();
    }

    imgui_shutdown();
}

void Application::record_command_buffer(VkCommandBuffer command_buffer, u32 image_index) const
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pNext = VK_NULL_HANDLE;
    begin_info.pInheritanceInfo = VK_NULL_HANDLE;

    vkBeginCommandBuffer(command_buffer, &begin_info); // command buffer

    const u32 width = m_Window->get_framebuffer_width();
    const u32 height = m_Window->get_framebuffer_height();

    const VkClearValue clear_color = {m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0f };
    const VkRect2D render_area = { { 0, 0 },{ width, height }};
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext           = VK_NULL_HANDLE;
    render_pass_begin_info.renderPass      = m_Vk->get_vk_render_pass();
    render_pass_begin_info.renderArea      = render_area;
    render_pass_begin_info.framebuffer     = m_Framebuffers[image_index];
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues    = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE); // render pass

    // create viewport
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Vk->get_vk_gfx_pipeline());
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Window->get_framebuffer_width());
        viewport.height = static_cast<float>(m_Window->get_framebuffer_height());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = { width, height };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
        vkCmdDraw(command_buffer, 3, 1, 0, 0);
    }

    // record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);

    vkCmdEndRenderPass(command_buffer); // !render pass

    vkEndCommandBuffer(command_buffer); // !command buffer
}

void Application::create_command_buffers()
{
    const u32 image_count = m_Vk->get_swapchain()->get_vk_image_count();
    m_CommandBuffers.resize(image_count);
    m_Vk->create_command_buffers(image_count, m_CommandBuffers.data());
}

void Application::imgui_init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    // setup renderer backends
    constexpr bool install_callbacks = true;
    ImGui_ImplGlfw_InitForVulkan(m_Window->get_native_window(), install_callbacks);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Vk->get_vk_instance();
    init_info.PhysicalDevice = m_Vk->get_vk_physical_device();
    init_info.Device = m_Vk->get_vk_logical_device();
    init_info.QueueFamily = m_Vk->get_vk_queue_family();
    init_info.Queue = m_Vk->get_queue()->get_vk_queue();
    init_info.PipelineCache = m_Vk->get_vk_pipeline_cache();
    init_info.DescriptorPool = m_Vk->get_vk_descriptor_pool();
    init_info.RenderPass = m_Vk->get_vk_render_pass();
    init_info.Subpass = 0;
    init_info.MinImageCount = m_Vk->get_swapchain()->get_vk_min_image_count();
    init_info.ImageCount = m_Vk->get_swapchain()->get_vk_image_count();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = m_Vk->get_vk_allocator();
    init_info.CheckVkResultFn = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&init_info);
}

void Application::imgui_begin()
{
    // i32 width = m_Window->get_framebuffer_width();
    // i32 height = m_Window->get_framebuffer_height();
    // TODO: Recreate swapchain
    // TODO: Dock space

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::imgui_end()
{
    ImGui::Render();
}

void Application::imgui_shutdown() const
{
    m_Vk->get_queue()->wait_idle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::present() const
{
    m_Vk->get_queue()->wait_idle();
    const u32 image_index = m_Vk->get_queue()->acquired_next_image();

    // wait for fences
    m_Vk->get_queue()->wait_and_reset_fences();

    // reset and record command buffer
    vkResetCommandBuffer(m_CommandBuffers[image_index], 0);
    record_command_buffer(m_CommandBuffers[image_index], image_index);

    // submit command buffer
    m_Vk->get_queue()->submit_async(m_CommandBuffers[image_index]);

    // present the image
    m_Vk->get_queue()->present(image_index);
}

