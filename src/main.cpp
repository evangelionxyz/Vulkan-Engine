// Copyright 2024, Evangelion Manuhutu

#include <backends/imgui_impl_vulkan.h>

#include "core/window.h"
#include "vulkan/vulkan_context.h"
#include "vulkan/vulkan_wrapper.h"
#include "core/logger.h"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.cpp>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui_internal.h>

int main()
{
    Logger logger;

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
        clear_value.color.float32[1] = 0.1f;
        clear_value.color.float32[2] = 0.5f;
        clear_value.color.float32[3] = 1.0f;

        const VkRect2D render_area = { { 0, 0 },
            { static_cast<u32>(window.get_width()), static_cast<u32>(window.get_height()) }};
        VkRenderPassBeginInfo render_pass_begin_info = {};
        render_pass_begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_begin_info.pNext           = VK_NULL_HANDLE;
        render_pass_begin_info.renderPass      = render_pass;
        render_pass_begin_info.renderArea      = render_area;
        render_pass_begin_info.clearValueCount = 1;
        render_pass_begin_info.pClearValues    = &clear_value;

        for (size_t i = 0; i < command_buffers.size(); ++i)
        {
            vk_begin_command_buffer(command_buffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
            render_pass_begin_info.framebuffer = frame_buffers[i];
            vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdEndRenderPass(command_buffers[i]);
            VK_ERROR_CHECK(vkEndCommandBuffer(command_buffers[i]),
                "[vkEndCommandBuffer] Failed to end command buffer");
        }

        LOG_INFO("Command buffers recorded");
    };

    create_frame_buffers();
    record_command_buffers();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // init imgui
    i32 width, height;
    glfwGetFramebufferSize(window.get_native_window(), &width, &height);
    ImGui_ImplVulkan_InitInfo imgui_init_info = {};
    imgui_init_info.Instance = vk.get_instance();
    imgui_init_info.PhysicalDevice = vk.get_physical_device();
    imgui_init_info.Device = vk.get_logical_device();
    imgui_init_info.QueueFamily = vk.get_queue_family();
    imgui_init_info.Queue = vk.get_queue()->get_queue();
    imgui_init_info.PipelineCache = vk.get_pipeline_cache();
    imgui_init_info.DescriptorPool = vk.get_descriptor_pool();
    imgui_init_info.MinImageCount = 2;
    imgui_init_info.ImageCount = vk.get_image_count();
    imgui_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imgui_init_info.Subpass = 1;
    imgui_init_info.RenderPass = render_pass;
    imgui_init_info.Allocator = vk.get_allocator();
    //ImGui_ImplVulkan_Init(&imgui_init_info);

    auto render_scene = [&]()
    {
        const u32 image_index = queue->acquired_next_image();
        queue->submit_async(command_buffers[image_index]);
        queue->present(image_index);
    };

    while (window.is_looping())
    {
        window.poll_events();
        //ImGui_ImplVulkan_NewFrame();
        //ImGui::NewFrame();
        //ImGui::Render();

        //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), 0);
        //ImGui::EndFrame();

        render_scene();
    }

    queue->wait_idle();
    //ImGui_ImplVulkan_Shutdown();

    vk.free_command_buffers(static_cast<u32>(command_buffers.size()), command_buffers.data());
    vk.destroy_framebuffers(frame_buffers);
    vkDestroyRenderPass(vk.get_logical_device(), render_pass, vk.get_allocator());
}
