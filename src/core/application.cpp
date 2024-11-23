// Copyright (c) 2024, Evangelion Manuhutu

#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "logger.hpp"
#include "vulkan/vulkan_context.hpp"
#include "vulkan/vulkan_wrapper.hpp"

Application::Application(i32 argc, char **argv)
{
    m_Window = CreateScope<Window>(1024, 720, "Vulkan Engine");
}

Application::~Application()
{
    m_Window->get_vk_context()->get_queue()->wait_idle();
    imgui_shutdown();
}

void Application::run()
{
    imgui_init();

    while (m_Window->is_looping())
    {
        m_Window->poll_events();
        {
            imgui_begin();
            ImGui::ShowDemoWindow();
            ImGui::Begin("Settings");
            ImGui::ColorEdit4("clear color", &m_ClearColor[0]);
            ImGui::End();
            imgui_end();

            m_Window->submit([&](VkCommandBuffer command_buffer)
            {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
            });
        }

        m_Window->present(m_ClearColor);
    }
}

void Application::imgui_init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoDecoration = false;

    // setup renderer backends
    constexpr bool install_callbacks = true;
    ImGui_ImplGlfw_InitForVulkan(m_Window->get_native_window(), install_callbacks);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Window->get_vk_context()->get_vk_instance();
    init_info.PhysicalDevice = m_Window->get_vk_context()->get_vk_physical_device();
    init_info.Device = m_Window->get_vk_context()->get_vk_logical_device();
    init_info.QueueFamily = m_Window->get_vk_context()->get_vk_queue_family();
    init_info.Queue = m_Window->get_vk_context()->get_queue()->get_vk_queue();
    init_info.PipelineCache = m_Window->get_vk_context()->get_vk_pipeline_cache();
    init_info.DescriptorPool = m_Window->get_vk_context()->get_vk_descriptor_pool();
    init_info.RenderPass = m_Window->get_vk_context()->get_vk_render_pass();
    init_info.Subpass = 0;
    init_info.MinImageCount = m_Window->get_vk_context()->get_swapchain()->get_vk_min_image_count();
    init_info.ImageCount = m_Window->get_vk_context()->get_swapchain()->get_vk_image_count();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = m_Window->get_vk_context()->get_vk_allocator();
    init_info.CheckVkResultFn = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&init_info);
}

void Application::imgui_begin()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static ImGuiDockNodeFlags doc_space_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PopStyleVar(2);

    if (doc_space_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    {
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 2));
    ImGui::Begin("ORigin", nullptr, window_flags);

    ImGuiStyle& style = ImGui::GetStyle();
    const float min_window_size_x = style.WindowMinSize.x;
    const float min_window_size_y = style.WindowMinSize.y;
    style.WindowMinSize.x = 220.0f;
    style.WindowMinSize.y = 38.0f;
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        const ImGuiID dock_space_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dock_space_id, ImVec2(0.0f, 0.0f), doc_space_flags);
    }
    style.WindowMinSize.x = min_window_size_x;
    style.WindowMinSize.y = min_window_size_y;
}

void Application::imgui_end()
{
    // docking window
    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::Render();

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void Application::imgui_shutdown() const
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

