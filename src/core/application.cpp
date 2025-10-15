// Copyright (c) 2024, Evangelion Manuhutu

#include "application.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <unordered_map>

#include "logger.hpp"

#include "vulkan/buffers.hpp"
#include "vulkan/command_buffer.hpp"
#include "vulkan/graphics_pipeline.hpp"
#include "vulkan/shader.hpp"

#include "vulkan/vulkan_context.hpp"
#include "vulkan/vulkan_wrapper.hpp"

Application::Application(i32 argc, char **argv)
{
    m_Window = CreateScope<Window>(1024, 720, "Vulkan Engine");

    m_Window->set_window_resize_callback([this](uint32_t width, uint32_t height) {
        on_window_resize(width, height);
    });
    m_Window->set_framebuffer_resize_callback([this](uint32_t width, uint32_t height) {
        on_framebuffer_resize(width, height);
    });

    m_Vk = m_Window->get_vk_context();

    m_CommandBuffer = CommandBuffer::create();

    glm::vec2 size = { static_cast<float>(m_Window->get_window_width()), static_cast<float>(m_Window->get_window_height())};

    m_Camera = Camera(45.0f, size.x, size.y);
    m_Camera.set_position(glm::vec3(0.0f, 0.0f, 5.0f)).update_view_matrix();

    create_graphics_pipeline();
}

Application::~Application()
{
    const VkDevice device = m_Vk->get_device();
    m_Vk->get_queue()->wait_idle();

    if (m_Pipeline)
    {
        m_Pipeline->destroy();
    }

    if (m_VertexBuffer)
    {
        m_VertexBuffer->destroy();
    }

    if (m_IndexBuffer)
    {
        m_IndexBuffer->destroy();
    }

    for (auto layout : m_DescLayouts)
    {
        vkDestroyDescriptorSetLayout(device, layout, VK_NULL_HANDLE);
    }

    if (m_CommandBuffer)
    {
        m_CommandBuffer->destroy();
    }

    imgui_shutdown();
}

void Application::run()
{
    imgui_init();

    SDL_Event event;

    while (m_Window->is_looping())
    {
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            m_Window->poll_events(&event);
        }

        float delta_time = SDL_GetTicks();
        on_update(delta_time);
        
        if (auto frame_index = m_Vk->begin_frame())
        {
            imgui_begin();
            ImGui::ShowDemoWindow();
            ImGui::Begin("Settings");
            ImGui::ColorEdit4("clear color", &m_ClearColor[0]);
            ImGui::End();
            imgui_end();

            record_frame(*frame_index);

            m_Vk->present();
        }
    }
}

void Application::on_update(float delta_time)
{
    m_Camera.update_view_matrix();
}

void Application::on_window_resize(uint32_t width, uint32_t height)
{
}

void Application::on_framebuffer_resize(uint32_t width, uint32_t height)
{
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    m_Camera.resize({w, h}).update_projection_matrix();
}

void Application::create_graphics_pipeline()
{
    const VkDevice device = m_Vk->get_device();

    const Ref<Shader> vertex_shader = CreateRef<Shader>("res/shaders/default.vert", VK_SHADER_STAGE_VERTEX_BIT);
    const Ref<Shader> fragment_shader = CreateRef<Shader>("res/shaders/default.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<Vertex> vertices =
    {
        // Position, Color (clockwise winding)
        {{ -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{ -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}
    };

    std::vector<uint32_t> indices = 
    {
        0, 1, 2,
        0, 2, 3
    };

    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();
    m_VertexBuffer = VertexBuffer::create(vertices.data(), buffer_size);
    m_IndexBuffer = IndexBuffer::create(indices);

    // Build vertex input state from reflection if available; fallback to hardcoded Vertex layout
    VkVertexInputBindingDescription binding_desc = {};
    std::vector<VkVertexInputAttributeDescription> attr_desc;
    const auto& reflected_attrs = vertex_shader->get_vertex_attributes();
    if (!reflected_attrs.empty())
    {
        binding_desc.binding = 0;
        binding_desc.stride = vertex_shader->get_vertex_stride();
        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        attr_desc = reflected_attrs; // copy
    }
    else
    {
        binding_desc = Vertex::get_vk_binding_desc();
        auto arr = Vertex::get_vk_attribute_desc();
        attr_desc.assign(arr.begin(), arr.end());
    }

    // Merge descriptor set layouts from both shaders
    std::unordered_map<u32, std::vector<VkDescriptorSetLayoutBinding>> merged_sets;
    auto merge_sets = [&merged_sets]
        (const std::unordered_map<u32, std::vector<VkDescriptorSetLayoutBinding>>& src)
    {
        for (const auto& [set, bindings] : src)
        {
            auto& dst_vec = merged_sets[set];
            for (const auto& b : bindings)
            {
                bool merged = false;
                for (auto& existing : dst_vec)
                {
                    if (existing.binding == b.binding && existing.descriptorType == b.descriptorType)
                    {
                        existing.stageFlags |= b.stageFlags; // merge stage flags
                        merged = true;
                        break;
                    }
                }
                if (!merged)
                {
                    dst_vec.push_back(b);
                }
            }
        }
    };

    merge_sets(vertex_shader->get_descriptor_set_layout_bindings());
    merge_sets(fragment_shader->get_descriptor_set_layout_bindings());

    // Create VkDescriptorSetLayout(s)
    std::vector<std::pair<u32, VkDescriptorSetLayout>> set_layout_pairs;
    set_layout_pairs.reserve(merged_sets.size());
    for (auto& [set_index, bindings] : merged_sets)
    {
        VkDescriptorSetLayoutCreateInfo set_info { };
        set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        set_info.bindingCount = static_cast<u32>(bindings.size());
        set_info.pBindings = bindings.data();
        VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
        VkResult res = vkCreateDescriptorSetLayout(device, &set_info, nullptr, &set_layout);
        VK_ERROR_CHECK(res, "[Vulkan] Failed to create descriptor set layout");
        set_layout_pairs.emplace_back(set_index, set_layout);
        m_DescLayouts.push_back(set_layout); // Store for cleanup
    }
    // Sort by set index and create array
    std::sort(set_layout_pairs.begin(), set_layout_pairs.end(), [](auto& a, auto& b){ return a.first < b.first; });
    std::vector<VkDescriptorSetLayout> set_layouts;
    set_layouts.reserve(set_layout_pairs.size());
    for (auto& p : set_layout_pairs) set_layouts.push_back(p.second);

    // Merge push constant ranges
    std::vector<VkPushConstantRange> push_ranges = vertex_shader->get_push_constant_ranges();
    for (const auto& pr : fragment_shader->get_push_constant_ranges())
    {
        bool merged = false;
        for (auto& ex : push_ranges)
        {
            if (ex.offset == pr.offset && ex.size == pr.size)
            {
                ex.stageFlags |= pr.stageFlags;
                merged = true;
                break;
            }
        }
        if (!merged)
            push_ranges.push_back(pr);
    }

    VkPipelineLayoutCreateInfo layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<u32>(set_layouts.size()),
        .pSetLayouts = set_layouts.empty() ? VK_NULL_HANDLE : set_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(push_ranges.size()),
        .pPushConstantRanges = push_ranges.empty() ? VK_NULL_HANDLE : push_ranges.data()
    };

    // create pipeline layout
    VkPipelineLayout pipeline_layout;
    VkResult result = vkCreatePipelineLayout(device, &layout_create_info, VK_NULL_HANDLE, &pipeline_layout);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create pipeline layout");

    GraphicsPipelineInfo pipeline_info {
        .binding_description = binding_desc,
        .attribute_descriptions = attr_desc,  // This copies the vector
        .layout = pipeline_layout,
        .extent = VulkanContext::get()->get_swap_chain()->get_extent(),
        .render_pass = m_Vk->get_render_pass(),
    };

    m_Pipeline = CreateRef<GraphicsPipeline>();
    m_Pipeline->add_shader(vertex_shader)
        .add_shader(fragment_shader)
        .build(pipeline_info);
}

void Application::record_frame(uint32_t frame_index)
{
    const VkExtent2D extent = m_Vk->get_swap_chain()->get_extent();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkClearValue clear_value{};
    clear_value.color.float32[0] = m_ClearColor.r;
    clear_value.color.float32[1] = m_ClearColor.g;
    clear_value.color.float32[2] = m_ClearColor.b;
    clear_value.color.float32[3] = m_ClearColor.a;

    m_CommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VkCommandBuffer command_buffer = m_CommandBuffer->get_active_handle();

    const glm::mat4 &view_projection = m_Camera.get_view_projection_matrix();
    m_CommandBuffer->set_push_constants(VK_SHADER_STAGE_VERTEX_BIT, m_Pipeline->get_layout(), &view_projection, sizeof(glm::mat4));

    GraphicsState state;
    state.pipeline = m_Pipeline->get_handle();
    state.framebuffer = m_Vk->get_framebuffer(frame_index);
    state.render_pass = m_Vk->get_render_pass();
    state.scissor = scissor;
    state.viewport = viewport;
    state.clear_value = clear_value;
    state.index_buffer = m_IndexBuffer->get_buffer();
    state.vertex_buffers = { m_VertexBuffer->get_buffer() };
    
    m_CommandBuffer->set_graphics_state(state);

    DrawArguments args;
    args.vertex_count = m_IndexBuffer->get_count();
    args.instance_count = 1;
    m_CommandBuffer->draw_indexed(args);

    if (ImDrawData* draw_data = ImGui::GetDrawData())
    {
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
    }

    m_CommandBuffer->end();
    m_Vk->submit({command_buffer});
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
        if (!sdl_window) return 1; // error
        bool ret = SDL_Vulkan_CreateSurface(sdl_window, (VkInstance)vk_instance, (const VkAllocationCallbacks*)vk_allocator, (VkSurfaceKHR*)out_vk_surface);
        return ret ? 0 : 1; // 0 on success as expected by imgui_impl_vulkan
    };

    ImGui::GetPlatformIO().Platform_CreateVkSurface = create_vk_surface;

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Vk->get_instance();
    init_info.PhysicalDevice = m_Vk->get_physical_device();
    init_info.Device = m_Vk->get_device();
    init_info.QueueFamily = m_Vk->get_queue_family();
    init_info.Queue = m_Vk->get_queue()->get_handle();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_Vk->get_descriptor_pool();
    init_info.RenderPass = m_Vk->get_render_pass();
    init_info.Subpass = 0;
    init_info.MinImageCount = m_Vk->get_swap_chain()->get_min_image_count();
    init_info.ImageCount = m_Vk->get_swap_chain()->get_image_count();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&init_info);

    if (!ImGui_ImplVulkan_CreateFontsTexture())
    {
        Logger::get_instance().push_message("[ImGui] Failed to create font texture", LoggingLevel::Error);
    }
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
    ImGui::Begin("Vulkan", nullptr, window_flags);

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

