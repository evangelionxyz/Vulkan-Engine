// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_context.hpp"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <unordered_map>

#include "core/assert.hpp"
#include "core/logger.hpp"

#include "vulkan_wrapper.hpp"
#include "vulkan_shader.hpp"

#include "core/window.hpp"

#include <SDL3/SDL_vulkan.h>

#ifdef __linux__
    #include <vulkan/vulkan_wayland.h>
#endif

#include <backends/imgui_impl_vulkan.h>

#include <queue>
#include <functional>
#include <ranges>

static VulkanContext *s_Instance = nullptr;

VulkanContext::VulkanContext(Window *window)
    : m_Window(window)
{
    s_Instance = this;

    Logger::get_instance().push_message("=== Initializing Vulkan ===");
    
    create_instance();
#ifdef VK_DEBUG
    create_debug_callback();
#endif
    create_window_surface();
    m_PhysicalDevice = VulkanPhysicalDevice(m_Instance, m_Surface);
    m_QueueFamily = m_PhysicalDevice.select_device(VK_QUEUE_GRAPHICS_BIT, true);

    create_device();
    create_swapchain();
    create_render_pass();
    create_command_pool();

    m_Queue = VulkanQueue(m_QueueFamily, 0);
    create_descriptor_pool();

    create_graphics_pipeline();

    create_framebuffers();

    create_command_buffers();
}

void VulkanContext::destroy()
{
    Logger::get_instance().push_message("=== Destroying Vulkan ===");

    m_Buffer->destroy();
    free_command_buffers();
    destroy_framebuffers();
    reset_command_pool();
    vkDestroyRenderPass(m_Device, m_RenderPass, VK_NULL_HANDLE);
    m_GraphicsPipeline->destroy();

    // Destroy descriptor set layouts
    for (auto layout : m_DescriptorSetLayouts)
    {
        vkDestroyDescriptorSetLayout(m_Device, layout, VK_NULL_HANDLE);
    }
    m_DescriptorSetLayouts.clear();

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, VK_NULL_HANDLE);
    vkDestroyCommandPool(m_Device, m_CommandPool, VK_NULL_HANDLE);

    m_Queue.destroy();
    m_SwapChain.destroy();

    vkDestroySurfaceKHR(m_Instance, m_Surface, VK_NULL_HANDLE);
    Logger::get_instance().push_message("[Vulkan] Window surface destroyed");

#ifdef VK_DEBUG
    // destroy debug messenger
    const auto dbg_messenger_func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    ASSERT(dbg_messenger_func, "[Vulkan] Cannot find address of vkDestroyDebugUtilsMessengerEXT");
    dbg_messenger_func(m_Instance, m_DebugMessenger, VK_NULL_HANDLE);
    Logger::get_instance().push_message("[Vulkan] Debug messenger destroyed");
#endif

    vkDestroyDevice(m_Device, VK_NULL_HANDLE);
    Logger::get_instance().push_message("[Vulkan] Logical device destroyed");

    vkDestroyInstance(m_Instance, VK_NULL_HANDLE);
    Logger::get_instance().push_message("[Vulkan] Instance destroyed");
}

void VulkanContext::create_render_pass()
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format         = m_SwapChain.get_format().format;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pInputAttachments = VK_NULL_HANDLE;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = VK_NULL_HANDLE;
    subpass.pResolveAttachments = VK_NULL_HANDLE;
    subpass.pDepthStencilAttachment = VK_NULL_HANDLE;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &color_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies   = &subpass_dependency;

    VkResult result = vkCreateRenderPass(m_Device, &render_pass_info, VK_NULL_HANDLE, &m_RenderPass);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create render pass");
    Logger::get_instance().push_message("[Vulkan] Render pass created");
}

void VulkanContext::destroy_framebuffers() const
{
    for (const auto framebuffer : m_MainFrameBuffers)
        vkDestroyFramebuffer(m_Device, framebuffer, VK_NULL_HANDLE);
}

void VulkanContext::reset_command_pool() const
{
    m_Queue.wait_idle();

    VkResult result = vkResetCommandPool(m_Device, m_CommandPool, 0);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to reset command pool");
}

void VulkanContext::set_clear_color(const glm::vec4 &clear_color)
{
    m_ClearValue.color.float32[0] = clear_color.r;
    m_ClearValue.color.float32[1] = clear_color.g;
    m_ClearValue.color.float32[2] = clear_color.b;
    m_ClearValue.color.float32[3] = clear_color.a;
}

VkInstance VulkanContext::get_instance() const
{
    return m_Instance;
}

VkPhysicalDevice VulkanContext::get_physical_device() const
{
    return m_PhysicalDevice.get_selected_device().device;
}

VkDescriptorPool VulkanContext::get_descriptor_pool() const
{
    return m_DescriptorPool;
}

VkDevice VulkanContext::get_device() const
{
    return m_Device;
}

VulkanQueue* VulkanContext::get_queue()
{
    return &m_Queue;
}

u32 VulkanContext::get_queue_family() const
{
    return m_QueueFamily;
}

VulkanSwapchain* VulkanContext::get_swap_chain()
{
    return &m_SwapChain;
}

VulkanContext *VulkanContext::get()
{
    return s_Instance;
}

void VulkanContext::create_command_buffers()
{
    const u32 image_count = m_SwapChain.get_image_count();
    m_MainCmdBuffers.resize(image_count);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandBufferCount = image_count;
    alloc_info.commandPool        = m_CommandPool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkResult result = vkAllocateCommandBuffers(m_Device, &alloc_info, m_MainCmdBuffers.data());
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create command buffers");

    Logger::get_instance().push_message("[Vulkan] Command buffers created");
}

void VulkanContext::free_command_buffers() const
{
    m_Queue.wait_idle();
    const u32 count = static_cast<u32>(m_MainCmdBuffers.size());
    vkFreeCommandBuffers(m_Device, m_CommandPool, count, m_MainCmdBuffers.data());
}

VkCommandPool VulkanContext::get_command_pool() const
{
    return m_CommandPool;
}

VkRenderPass VulkanContext::get_render_pass() const
{
    return m_RenderPass;
}

void VulkanContext::create_instance()
{
    VkApplicationInfo app_info = {};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Vulkan Application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "Vulkan Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 0);

    uint32_t property_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &property_count, nullptr);
    std::vector<VkExtensionProperties> properties(property_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &property_count, properties.data());
    for (const auto & [extensionName, specVersion] : properties)
        LOG_INFO("[Vulkan] Instance extension: {}", extensionName);

    u32 req_extension_count = 0;
    const char * const *req_extensions = SDL_Vulkan_GetInstanceExtensions(&req_extension_count);
    std::vector<const char *> extensions;
    extensions.reserve(req_extension_count);

    for (u32 i = 0; i < req_extension_count; ++i)
    {
        extensions.push_back(req_extensions[i]);
        LOG_INFO("[Vulkan] Required extension: {}", req_extensions[i]);
    }


    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    extensions.push_back("VK_KHR_win32_surface");
#elif __linux__
    extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    std::vector<const char *>layers{};
#ifdef VK_DEBUG
    layers.push_back("VK_LAYER_KHRONOS_validation");
    Logger::get_instance().push_message("[Vulkan] Validation layer enabled");
#endif

    // Setup debug messenger for instance creation/destruction
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
#ifdef VK_DEBUG
    debug_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = vk_debug_messenger_callback;
    debug_create_info.pUserData       = VK_NULL_HANDLE;
#endif

    VkInstanceCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = static_cast<u32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount       = std::size(layers);
    create_info.ppEnabledLayerNames     = layers.data();
#ifdef VK_DEBUG
    create_info.pNext                   = &debug_create_info;
#endif

    const VkResult res = vkCreateInstance(&create_info, VK_NULL_HANDLE, &m_Instance);
    VK_ERROR_CHECK(res, "[Vulkan] Failed to create instance");
    Logger::get_instance().push_message("[Vulkan] Vulkan instance created");
}

void VulkanContext::create_debug_callback()
{
    VkDebugUtilsMessengerCreateInfoEXT msg_create_info = {};
    msg_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    msg_create_info.pNext           = VK_NULL_HANDLE;
    msg_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    msg_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    msg_create_info.pfnUserCallback = vk_debug_messenger_callback;
    msg_create_info.pUserData       = VK_NULL_HANDLE;

    const auto dbg_messenger_func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT(dbg_messenger_func, "[Vulkan] Cannot find address of vkCreateDebugUtilsMessengerEXT");

    const VkResult result = dbg_messenger_func(m_Instance, &msg_create_info, VK_NULL_HANDLE, &m_DebugMessenger);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create debug messenger");
    Logger::get_instance().push_message("[Vulkan] Debug utils messenger created");
}

void VulkanContext::create_window_surface()
{
    const bool res = SDL_Vulkan_CreateSurface(m_Window->get_native_window(), m_Instance, VK_NULL_HANDLE, &m_Surface);
    ASSERT(res, "[Vulkan] Failed to create window surface");
    Logger::get_instance().push_message("[Vulkan] Window surface created");
}

void VulkanContext::create_device()
{
    constexpr float queue_priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = m_QueueFamily,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    };

    const char *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME };

    if (m_PhysicalDevice.get_selected_device().features.geometryShader == VK_FALSE)
        Logger::get_instance().push_message("[Vulkan] Geometry shader is not supported", LoggingLevel::Error);

    if (m_PhysicalDevice.get_selected_device().features.tessellationShader == VK_FALSE)
        Logger::get_instance().push_message("[Vulkan] Tessellation shader is not supported", LoggingLevel::Error);

    VkPhysicalDeviceFeatures device_features = { 0 };
    device_features.geometryShader = VK_TRUE;
    device_features.tessellationShader = VK_TRUE;

    VkDeviceCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.flags                   = 0;
    create_info.queueCreateInfoCount    = 1;
    create_info.pQueueCreateInfos       = &queue_create_info;
    create_info.pNext                   = VK_NULL_HANDLE;
    create_info.enabledLayerCount       = 0;
    create_info.ppEnabledLayerNames     = VK_NULL_HANDLE;
    create_info.enabledExtensionCount   = std::size(device_extensions);
    create_info.ppEnabledExtensionNames = device_extensions;
    create_info.pEnabledFeatures        = &device_features;

    const VkResult result = vkCreateDevice(m_PhysicalDevice.get_selected_device().device, &create_info, VK_NULL_HANDLE, &m_Device);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create logical device");
    Logger::get_instance().push_message("[Vulkan] Logical device created");
}

void VulkanContext::create_swapchain()
{
    const u32 width = m_Window->get_framebuffer_width();
    const u32 height = m_Window->get_framebuffer_height();

    VkSurfaceCapabilitiesKHR capabilities = VulkanPhysicalDevice::get_surface_capabilities(m_PhysicalDevice.get_selected_device().device, m_Surface);

    const VkExtent2D swap_chain_extent = { width, height };

    capabilities.currentExtent.width = std::clamp(
        swap_chain_extent.width,
        capabilities.minImageExtent.width, 
        capabilities.maxImageExtent.width
    );

    capabilities.currentExtent.height = std::clamp(
        swap_chain_extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    const std::vector<VkPresentModeKHR> &present_modes = m_PhysicalDevice.get_selected_device().present_modes;
    const VkPresentModeKHR present_mode = vk_choose_present_mode(present_modes);

    const VkSurfaceFormatKHR format = vk_choose_surface_format(m_PhysicalDevice.get_selected_device().surface_formats);
    const VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    m_SwapChain = VulkanSwapchain(m_Surface, format, capabilities, present_mode, imageUsage, m_QueueFamily);
}

void VulkanContext::create_command_pool()
{
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = m_QueueFamily;

    VK_ERROR_CHECK(vkCreateCommandPool(m_Device, &pool_create_info, VK_NULL_HANDLE, &m_CommandPool),
        "[Vulkan] Failed to create command pool");

    Logger::get_instance().push_message("[Vulkan] Command buffer created");
}

void VulkanContext::create_descriptor_pool()
{
    const VkDescriptorPoolSize pool_sizes[] =
    {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 1000 * std::size(pool_sizes);
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes    = pool_sizes;

    VkResult result = vkCreateDescriptorPool(m_Device, &pool_info, VK_NULL_HANDLE, &m_DescriptorPool);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create command pool");
}

void VulkanContext::create_graphics_pipeline()
{
    const Ref<VulkanShader> vertex_shader = CreateRef<VulkanShader>("res/shaders/default.vert", VK_SHADER_STAGE_VERTEX_BIT);
    const Ref<VulkanShader> fragment_shader = CreateRef<VulkanShader>("res/shaders/default.frag", VK_SHADER_STAGE_FRAGMENT_BIT);

    std::vector<Vertex> vertices =
    {
        {{ -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Position, Color
        {{ 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  -0.5f}, {0.0f, 0.0f, 1.0f}}
    };
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

    m_Buffer = CreateRef<VulkanVertexBuffer>(m_Device, m_PhysicalDevice.get_selected_device().device, buffer_size, VK_NULL_HANDLE);
    copy_data_to_buffer(m_Device, m_Buffer->get_buffer_memory(), vertices.data(), buffer_size);

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
    auto merge_sets = [&merged_sets](const std::unordered_map<u32, std::vector<VkDescriptorSetLayoutBinding>>& src)
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
                    dst_vec.push_back(b);
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
        VkResult res = vkCreateDescriptorSetLayout(m_Device, &set_info, nullptr, &set_layout);
        VK_ERROR_CHECK(res, "[Vulkan] Failed to create descriptor set layout");
        set_layout_pairs.emplace_back(set_index, set_layout);
        m_DescriptorSetLayouts.push_back(set_layout); // Store for cleanup
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
        .pSetLayouts = set_layouts.empty() ? nullptr : set_layouts.data(),
        .pushConstantRangeCount = static_cast<u32>(push_ranges.size()),
        .pPushConstantRanges = push_ranges.empty() ? nullptr : push_ranges.data()
    };

    // create pipeline layout
    VkPipelineLayout pipeline_layout;
    VkResult result = vkCreatePipelineLayout(m_Device, &layout_create_info, nullptr, &pipeline_layout);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create pipeline layout");

    VulkanGraphicsPipelineInfo pipeline_info {
        .binding_description = binding_desc,
        .attribute_descriptions = attr_desc,  // This copies the vector
        .layout = pipeline_layout,
        .extent = m_SwapChain.get_extent(),
    };

    m_GraphicsPipeline = CreateRef<VulkanGraphicsPipeline>(m_RenderPass);
    m_GraphicsPipeline->add_shader(vertex_shader)
        .add_shader(fragment_shader)
        .build(pipeline_info);
}

void VulkanContext::create_framebuffers()
{
    const auto extent = m_SwapChain.get_extent();

    const u32 image_count = m_SwapChain.get_image_count();

    m_MainFrameBuffers.resize(image_count);

    for (u32 i = 0; i < image_count; i++)
    {
        VkImageView attachments[] = { m_SwapChain.get_image_view(i) };
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass      = m_RenderPass;
        framebuffer_create_info.width           = extent.width;
        framebuffer_create_info.height          = extent.height;
        framebuffer_create_info.layers          = 1;
        framebuffer_create_info.pAttachments    = attachments;
        framebuffer_create_info.attachmentCount = std::size(attachments);

        VkResult result = vkCreateFramebuffer(m_Device, &framebuffer_create_info, VK_NULL_HANDLE, &m_MainFrameBuffers[i]);
        VK_ERROR_CHECK(result, "[Vulkan] Failed to create framebuffer");
    }
    Logger::get_instance().push_message("[Vulkan] Frame buffer created");
}

void VulkanContext::recreate_swap_chain()
{
    vkDeviceWaitIdle(m_Device);
    destroy_framebuffers();
    m_SwapChain.destroy();
    create_swapchain();
    create_framebuffers();
}

void VulkanContext::record_command_buffer(VkCommandBuffer command_buffer, u32 image_index)
{
    // 1. begin command buffer
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pNext = VK_NULL_HANDLE;
    begin_info.pInheritanceInfo = VK_NULL_HANDLE;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    // 2. begin render pass
    const auto extent = m_SwapChain.get_extent();
    m_GraphicsPipeline->begin(command_buffer, m_MainFrameBuffers[image_index], m_ClearValue, extent);

    // 3. begin pipeline
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline->get_handle());

    const VkViewport viewport {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = static_cast<float>(extent.width),
        .height   = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = extent
    };

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { m_Buffer->get_buffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    m_GraphicsPipeline->end(command_buffer);

    vkEndCommandBuffer(command_buffer); // !command buffer
}

void VulkanContext::present()
{
    m_Queue.wait_idle();

    u32 image_index = 0;
    VkResult result = m_SwapChain.acquire_next_image(&image_index, m_Queue.get_semaphore());
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("[Vulkan] Failed to acquire SwapChain image");
    }

    m_Queue.wait_and_reset_fences();
    vkResetCommandBuffer(m_MainCmdBuffers[image_index], 0);
    record_command_buffer(m_MainCmdBuffers[image_index], image_index);

    m_Queue.submit_async(m_MainCmdBuffers[image_index]);

    result = m_Queue.present(image_index, m_SwapChain.get_handle());
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreate_swap_chain();
    }
}