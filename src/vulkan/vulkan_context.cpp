// Copyright 2024, Evangelion Manuhutu
#include "vulkan_context.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include "vulkan_wrapper.h"
#include "core/assert.h"
#include "core/logger.h"

#include <GLFW/glfw3.h>

static VulkanContext *s_Instance = nullptr;
VulkanContext::VulkanContext(GLFWwindow* window)
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
    m_Queue = VulkanQueue(m_LogicalDevice, *m_Swapchain.get_vk_swapchain(), m_Allocator, m_QueueFamily, 0);
    create_descriptor_pool();
}

VulkanContext::~VulkanContext()
{
    Logger::get_instance().push_message("=== Destroying Vulkan ===");

    // reset command pool
    reset_command_pool();

    // destroy render pass
    vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, m_Allocator);

    // destroy pipeline
    vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, m_Allocator);

    // destroy pipeline layout
    vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, m_Allocator);

    // destroy descriptor pool
    vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, m_Allocator);

    // destroy command  pool
    vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, m_Allocator);
    Logger::get_instance().push_message("[Vulkan] Command pool destroyed");

    // destroy semaphores
    m_Queue.destroy();
    Logger::get_instance().push_message("[Vulkan] Semaphores destroyed");

    // destroy swapchain
    m_Swapchain.destroy(m_LogicalDevice, m_Allocator);

    // destroy surface
    vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
    Logger::get_instance().push_message("[Vulkan] Window surface destroyed");

#ifdef VK_DEBUG
    // destroy debug messenger
    const auto dbg_messenger_func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    ASSERT(dbg_messenger_func, "[Vulkan] Cannot find address of vkDestroyDebugUtilsMessengerEXT");
    dbg_messenger_func(m_Instance, m_DebugMessenger, m_Allocator);
    Logger::get_instance().push_message("[Vulkan] Debug messenger destroyed");
#endif

    // destroy device
    vkDestroyDevice(m_LogicalDevice, m_Allocator);
    Logger::get_instance().push_message("[Vulkan] Logical device destroyed");

    // destroy instance
    vkDestroyInstance(m_Instance, m_Allocator);
    Logger::get_instance().push_message("[Vulkan] Instance destroyed");
}

std::vector<VkFramebuffer> VulkanContext::create_framebuffers(const u32 width, const u32 height) const
{
    const u32 image_count = static_cast<u32>(m_Swapchain.get_vk_image_count());
    std::vector<VkFramebuffer> frame_buffers(image_count);

    for (u32 i = 0; i < image_count; i++)
    {
        VkImageView attachments[] = { m_Swapchain.get_vk_image_view(i) };
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass      = m_RenderPass;
        framebuffer_create_info.width           = width;
        framebuffer_create_info.height          = height;
        framebuffer_create_info.layers          = 1;
        framebuffer_create_info.pAttachments    = attachments;
        framebuffer_create_info.attachmentCount = std::size(attachments);

        VK_ERROR_CHECK(vkCreateFramebuffer(m_LogicalDevice, &framebuffer_create_info, m_Allocator, &frame_buffers[i]),
            "[Vulkan] Failed to create framebuffer");
    }
    Logger::get_instance().push_message("[Vulkan] Framebuffers created");
    return frame_buffers;
}

void VulkanContext::create_render_pass()
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format         = m_Swapchain.get_vk_format().format;
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

    VK_ERROR_CHECK(vkCreateRenderPass(m_LogicalDevice, &render_pass_info, m_Allocator, &m_RenderPass),
                   "[Vulkan] Failed to create render pass");

    Logger::get_instance().push_message("[Vulkan] Render pass created");
}

void VulkanContext::destroy_framebuffers(const std::vector<VkFramebuffer> &frame_buffers) const
{
    for (const auto framebuffer : frame_buffers)
        vkDestroyFramebuffer(m_LogicalDevice, framebuffer, m_Allocator);
}

void VulkanContext::reset_command_pool() const
{
    m_Queue.wait_idle();

    VK_ERROR_CHECK(vkResetCommandPool(m_LogicalDevice, m_CommandPool, 0),
        "[Vulkan] Failed to reset command pool");
}

void VulkanContext::recreate_swapchain()
{
    m_RebuildSwapchain = false;
    m_Queue.wait_idle();

    // TODO: destroy framebufers

    // destroy old swapchain
    m_Swapchain.destroy(m_LogicalDevice, m_Allocator);

    // query capabilities

}

VkInstance VulkanContext::get_vk_instance() const
{
    return m_Instance;
}

VkPhysicalDevice VulkanContext::get_vk_physical_device() const
{
    return m_PhysicalDevice.get_selected_device().PhysDevice;
}

VkDescriptorPool VulkanContext::get_vk_descriptor_pool()
{
    return m_DescriptorPool;
}

VkPipelineCache VulkanContext::get_vk_pipeline_cache() const
{
    return m_PipelineCache;
}

VkPipelineLayout VulkanContext::get_vk_pipeline_layout() const
{
    return m_PipelineLayout;
}

VkDevice VulkanContext::get_vk_logical_device() const
{
    return m_LogicalDevice;
}

VulkanQueue* VulkanContext::get_queue()
{
    return &m_Queue;
}

VkPipeline VulkanContext::get_vk_gfx_pipeline() const
{
    return m_GraphicsPipeline;
}

u32 VulkanContext::get_vk_queue_family() const
{
    return m_QueueFamily;
}

VulkanSwapchain* VulkanContext::get_swapchain()
{
    return &m_Swapchain;
}

bool VulkanContext::is_rebuild_swapchain() const
{
    return m_RebuildSwapchain;
}

void VulkanContext::push_shader_create_info(VkShaderStageFlagBits stage, const VkPipelineShaderStageCreateInfo& create_info)
{
    m_ShaderStages[stage] = create_info;
}

VulkanContext *VulkanContext::get_instance()
{
    return s_Instance;
}

void VulkanContext::create_command_buffers(u32 count, VkCommandBuffer* command_buffers) const
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandBufferCount = count;
    alloc_info.commandPool        = m_CommandPool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_ERROR_CHECK(vkAllocateCommandBuffers(m_LogicalDevice, &alloc_info, command_buffers),
        "[Vulkan] Failed to create command buffers");
    Logger::get_instance().push_message("[Vulkan] Command buffers created");
}

void VulkanContext::free_command_buffers(std::vector<VkCommandBuffer> &command_buffers) const
{
    m_Queue.wait_idle();
    const u32 count = static_cast<u32>(command_buffers.size());
    vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, count, command_buffers.data());
}

VkAllocationCallbacks* VulkanContext::get_vk_allocator()
{
    return m_Allocator;
}

VkCommandPool VulkanContext::get_vk_command_pool() const
{
    return m_CommandPool;
}

VkRenderPass VulkanContext::get_vk_render_pass() const
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

    u32 req_extension_count = 0;
    const char **req_extensions = glfwGetRequiredInstanceExtensions(&req_extension_count);
    std::vector<const char *> extensions;
    extensions.reserve(req_extension_count);

    for (u32 i = 0; i < req_extension_count; ++i)
        extensions.push_back(req_extensions[i]);

    extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    extensions.push_back("VK_KHR_win32_surface");
#elif __linux__
    extensions.push_back("VK_KHR_xcb_surface");
#endif
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
    VkInstanceCreateInfo create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = static_cast<u32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount       = std::size(layers);
    create_info.ppEnabledLayerNames     = layers;

    const VkResult res = vkCreateInstance(&create_info, m_Allocator, &m_Instance);
    VK_ERROR_CHECK(res, "[Vulkan] Failed to create instance");
    Logger::get_instance().push_message("[Vulkan] Vulkan instance created");
}

void VulkanContext::create_debug_callback()
{
    VkDebugUtilsMessengerCreateInfoEXT msg_create_info = {};
    msg_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    msg_create_info.pNext           = VK_NULL_HANDLE;
    msg_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    msg_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    msg_create_info.pfnUserCallback = vk_debug_messenger_callback;
    msg_create_info.pUserData       = VK_NULL_HANDLE;

    const auto dbg_messenger_func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT(dbg_messenger_func, "[Vulkan] Cannot find address of vkCreateDebugUtilsMessengerEXT");

    const VkResult result = dbg_messenger_func(m_Instance, &msg_create_info, m_Allocator, &m_DebugMessenger);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create debug messenger");
    Logger::get_instance().push_message("[Vulkan] Debug utils messenger created");
}

void VulkanContext::create_window_surface()
{
    const VkResult res = glfwCreateWindowSurface(m_Instance, m_Window, m_Allocator, &m_Surface);
    VK_ERROR_CHECK(res, "[Vulkan] Failed to create window surface");
    Logger::get_instance().push_message("[Vulkan] Window surface created");
}

void VulkanContext::create_device()
{
    const float queue_priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pQueuePriorities = queue_priorities;
    queue_create_info.queueFamilyIndex = m_QueueFamily;
    queue_create_info.queueCount       = 1;

    const char *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME };

    if (m_PhysicalDevice.get_selected_device().Features.geometryShader == VK_FALSE)
        Logger::get_instance().push_message("[Vulkan] Geometry shader is not supported", LoggingLevel::Error);

    if (m_PhysicalDevice.get_selected_device().Features.tessellationShader == VK_FALSE)
        Logger::get_instance().push_message("[Vulkan] Tesselation shader is not supported", LoggingLevel::Error);

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

    const VkResult result = vkCreateDevice(m_PhysicalDevice.get_selected_device().PhysDevice, &create_info, m_Allocator, &m_LogicalDevice);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create logical device");
    Logger::get_instance().push_message("[Vulkan] Logical device created");
}

void VulkanContext::create_swapchain()
{
    i32 width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
    VkSurfaceCapabilitiesKHR capabilities = m_PhysicalDevice.get_selected_device().SurfaceCapabilities;
    const VkExtent2D swapchain_extent = { static_cast<u32>(width), static_cast<u32>(height) };
    capabilities.currentExtent.width = std::clamp(swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    capabilities.currentExtent.height = std::clamp(swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    const std::vector<VkPresentModeKHR> &present_modes = m_PhysicalDevice.get_selected_device().PresentModes;
    const VkPresentModeKHR present_mode = vk_choose_present_mode(present_modes);

    VkSurfaceFormatKHR format = vk_choose_surface_format(m_PhysicalDevice.get_selected_device().SurfaceFormats);
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    m_Swapchain = VulkanSwapchain(m_LogicalDevice, m_Allocator, m_Surface, format, capabilities, present_mode, image_usage, m_QueueFamily);
}

void VulkanContext::create_command_pool()
{
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = m_QueueFamily;

    VK_ERROR_CHECK(vkCreateCommandPool(m_LogicalDevice, &pool_create_info, m_Allocator, &m_CommandPool),
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

    VK_ERROR_CHECK(vkCreateDescriptorPool(m_LogicalDevice, &pool_info, m_Allocator, &m_DescriptorPool),
        "[Vulkan] Failed to create command pool");
}

void VulkanContext::create_graphics_pipeline()
{
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.vertexBindingDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_info = {};
    rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_info.depthClampEnable = VK_FALSE;
    rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_info.lineWidth = 1.0f;
    rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.sampleShadingEnable = VK_FALSE;
    multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_info = {};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.blendConstants[0] = 0.0f;
    color_blend_info.blendConstants[1] = 0.0f;
    color_blend_info.blendConstants[2] = 0.0f;
    color_blend_info.blendConstants[3] = 0.0f;

    std::vector dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = std::size(dynamic_states);
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    vkCreatePipelineLayout(m_LogicalDevice, &pipeline_layout_info, nullptr, &m_PipelineLayout);

    // create graphics pipeline
    std::vector<VkPipelineShaderStageCreateInfo> shader_create_infos;
    shader_create_infos.reserve(m_ShaderStages.size());
    for (auto [stage, create_info] : m_ShaderStages)
            shader_create_infos.push_back(create_info);

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};;
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = static_cast<u32>(shader_create_infos.size());
    pipeline_create_info.pStages = shader_create_infos.data();
    pipeline_create_info.pVertexInputState = &vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_info;
    pipeline_create_info.pViewportState = &viewport_info;
    pipeline_create_info.pRasterizationState = &rasterization_info;
    pipeline_create_info.pRasterizationState = &rasterization_info;
    pipeline_create_info.pMultisampleState = &multisample_info;
    pipeline_create_info.pColorBlendState = &color_blend_info;
    pipeline_create_info.pDynamicState = &dynamic_info;
    pipeline_create_info.pDepthStencilState = VK_NULL_HANDLE;
    pipeline_create_info.layout = m_PipelineLayout;
    pipeline_create_info.renderPass = m_RenderPass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_info.basePipelineIndex = -1;

    VK_ERROR_CHECK(vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipeline_create_info,
        m_Allocator, &m_GraphicsPipeline),
        "[Vulkan] Failed to create graphics pipeline");

}
