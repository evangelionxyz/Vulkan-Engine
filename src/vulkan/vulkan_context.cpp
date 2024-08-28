// Copyright 2024, Evangelion Manuhutu

#include "vulkan_context.h"

#include <cstdio>
#include <iterator>
#include "vulkan_wrapper.h"
#include "core/assert.h"

VulkanContext::VulkanContext(GLFWwindow* window)
    : m_Window(window)
{
    LOG_INFO("=== Initializing Vulkan ===");
    create_instance();
    //create_debug_callback();

    create_window_surface();
    m_PhysicalDevice = VulkanPhysicalDevice(m_Instance, m_Surface);
    m_QueueFamily = m_PhysicalDevice.select_device(VK_QUEUE_GRAPHICS_BIT, true);
    create_device();
    create_swapchain();
    create_command_pool();
    create_descriptor_pool();
    m_Queue = VulkanQueue(m_LogicalDevice, *m_Swapchain.get_vk_swapchain(), m_Allocator, m_QueueFamily, 0);
    create_graphics_pipeline();
}

VulkanContext::~VulkanContext()
{
    LOG_INFO("=== Destroying Vulkan ===");

    // destroy descriptor pool
    vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, m_Allocator);

    // destroy command  pool
    vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, m_Allocator);
    LOG_INFO("[Vulkan] Command pool destroyed");

    // destroy semaphores
    m_Queue.destroy();
    LOG_INFO("[Vulkan] Semaphores destroyed");

    // destroy swapchain
    m_Swapchain.destroy(m_LogicalDevice, m_Allocator);

    // destroy surface
    vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
    LOG_INFO("[Vulkan] Window surface destroyed");

    // destroy debug messenger
    PFN_vkDestroyDebugUtilsMessengerEXT vk_destroy_debug_utils_messenger = VK_NULL_HANDLE;
    vk_destroy_debug_utils_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    ASSERT(vk_destroy_debug_utils_messenger, "[Vulkan] Cannot find address of vkDestroyDebugUtilsMessengerEXT");
    vk_destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, m_Allocator);
    LOG_INFO("[Vulkan] Debug messenger destroyed");

    // destroy device
    vkDestroyDevice(m_LogicalDevice, m_Allocator);
    LOG_INFO("[Vulkan] Logical device destroyed");

    // destroy instance
    vkDestroyInstance(m_Instance, m_Allocator);
    LOG_INFO("[Vulkan] Instance destroyed");
}

std::vector<VkFramebuffer> VulkanContext::create_framebuffers(const VkRenderPass render_pass,
    const u32 width, const u32 height) const
{
    const u32 image_count = static_cast<u32>(m_Swapchain.get_vk_image_count());
    std::vector<VkFramebuffer> frame_buffers(image_count);

    for (u32 i = 0; i < image_count; i++)
    {
        VkImageView attachments[] = { m_Swapchain.get_vk_image_view(i) };
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass      = render_pass;
        framebuffer_create_info.width           = width;
        framebuffer_create_info.height          = height;
        framebuffer_create_info.layers          = 1;
        framebuffer_create_info.pAttachments    = attachments;
        framebuffer_create_info.attachmentCount = std::size(attachments);

        VK_ERROR_CHECK(vkCreateFramebuffer(m_LogicalDevice, &framebuffer_create_info, m_Allocator, &frame_buffers[i]),
            "[Vulkan] Failed to create framebuffer");
    }
    LOG_INFO("[Vulkan] Framebuffers created");
    return frame_buffers;
}

VkRenderPass VulkanContext::create_render_pass() const
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

    VkSubpassDescription sub_pass = {};
    sub_pass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub_pass.colorAttachmentCount = 1;
    sub_pass.pColorAttachments = &color_attachment_ref;
    sub_pass.pInputAttachments = nullptr;
    sub_pass.preserveAttachmentCount = 0;
    sub_pass.pPreserveAttachments = nullptr;
    sub_pass.pResolveAttachments = nullptr;
    sub_pass.pDepthStencilAttachment = nullptr;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &color_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &sub_pass;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VK_ERROR_CHECK(vkCreateRenderPass(m_LogicalDevice, &render_pass_info, m_Allocator, &render_pass),
                   "[Vulkan] Failed to create render pass");

    LOG_INFO("[Vulkan] Render pass created");
    return render_pass;
}

void VulkanContext::destroy_render_pass(VkRenderPass render_pass) const
{
    vkDestroyRenderPass(m_LogicalDevice, render_pass, m_Allocator);
}

void VulkanContext::destroy_framebuffers(const std::vector<VkFramebuffer> &frame_buffers) const
{
    for (const auto framebuffer : frame_buffers)
        vkDestroyFramebuffer(m_LogicalDevice, framebuffer, m_Allocator);
}

void VulkanContext::reset_command_pool() const
{
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

VkPipeline VulkanContext::get_vk_pipeline() const
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

void VulkanContext::create_command_buffers(u32 count, VkCommandBuffer* command_buffers) const
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandBufferCount = count;
    alloc_info.commandPool        = m_CommandPool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_ERROR_CHECK(vkAllocateCommandBuffers(m_LogicalDevice, &alloc_info, command_buffers),
        "[Vulkan] Failed to create command buffers");
    LOG_INFO("[Vulkan] Command buffers created");
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
    LOG_INFO("[Vulkan] Vulkan instance created");
}

void VulkanContext::create_debug_callback()
{
    VkDebugUtilsMessengerCreateInfoEXT msg_create_info = {};
    msg_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    msg_create_info.pNext           = VK_NULL_HANDLE;
    msg_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
    msg_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    msg_create_info.pfnUserCallback = VK_NULL_HANDLE;
    msg_create_info.pUserData       = VK_NULL_HANDLE;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
    vkCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT(vkCreateDebugUtilsMessenger, "[Vulkan] Cannot find address of vkCreateDebugUtilsMessenger");

    const VkResult result = vkCreateDebugUtilsMessenger(m_Instance, &msg_create_info, m_Allocator, &m_DebugMessenger);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create debug messenger");
    LOG_INFO("[Vulkan] Debug utils messenger created");
}

void VulkanContext::create_window_surface()
{
    const VkResult res = glfwCreateWindowSurface(m_Instance, m_Window, m_Allocator, &m_Surface);
    VK_ERROR_CHECK(res, "[Vulkan] Failed to create window surface");
    LOG_INFO("[Vulkan] Window surface created");
}

void VulkanContext::create_device()
{
    float queue_priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pQueuePriorities = queue_priorities;
    queue_create_info.queueFamilyIndex = m_QueueFamily;
    queue_create_info.queueCount       = 1;

    const char *device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME };

    if (m_PhysicalDevice.get_selected_device().Features.geometryShader == VK_FALSE)
        LOG_ERROR("[Vulkan] Geometry shader is not supported");

    if (m_PhysicalDevice.get_selected_device().Features.tessellationShader == VK_FALSE)
        LOG_ERROR("[Vulkan] Tesselation shader is not supported");

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
    LOG_INFO("[Vulkan] Logical device created");
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

    LOG_INFO("[Vulkan] Command buffer created");
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
    // TODO: Implement this
}
