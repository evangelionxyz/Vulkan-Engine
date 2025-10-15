// Copyright (c) 2024, Evangelion Manuhutu

#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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
    ImGui_ImplSDL3_InitForVulkan(m_Window->get_native_window());

    // Ensure Platform_CreateVkSurface is set (needed for multi-viewports with Vulkan)
    // We use SDL3 to create VkSurface for secondary viewports.
    auto create_vk_surface = [](ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface) -> int
    {
        SDL_WindowID win_id = (SDL_WindowID)(intptr_t)viewport->PlatformHandle;
        SDL_Window* sdl_window = SDL_GetWindowFromID(win_id);
        if (!sdl_window)
            return 1; // error
        bool ret = SDL_Vulkan_CreateSurface(sdl_window, (VkInstance)vk_instance, (const VkAllocationCallbacks*)vk_allocator, (VkSurfaceKHR*)out_vk_surface);
        return ret ? 0 : 1; // 0 on success as expected by imgui_impl_vulkan
    };
    ImGui::GetPlatformIO().Platform_CreateVkSurface = create_vk_surface;

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Window->get_vk_context()->get_instance();
    init_info.PhysicalDevice = m_Window->get_vk_context()->get_physical_device();
    init_info.Device = m_Window->get_vk_context()->get_device();
    init_info.QueueFamily = m_Window->get_vk_context()->get_queue_family();
    init_info.Queue = m_Window->get_vk_context()->get_queue()->get_handle();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_Window->get_vk_context()->get_descriptor_pool();
    init_info.RenderPass = m_Window->get_vk_context()->get_render_pass();
    init_info.Subpass = 0;
    init_info.MinImageCount = m_Window->get_vk_context()->get_swap_chain()->get_min_image_count();
    init_info.ImageCount = m_Window->get_vk_context()->get_swap_chain()->get_image_count();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&init_info);

}

void Application::imgui_begin()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
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
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

