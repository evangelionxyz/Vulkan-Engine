// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_context.hpp"

#include <algorithm>
#include <cstdio>
#include <iterator>

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "vulkan_wrapper.hpp"

#include "core/window.hpp"

#include <SDL3/SDL_vulkan.h>

#ifdef __linux__
    #include <vulkan/vulkan_wayland.h>
#endif

#include <backends/imgui_impl_vulkan.h>

#include <stdexcept>

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

    create_framebuffers();
}

void VulkanContext::destroy()
{
    Logger::get_instance().push_message("=== Destroying Vulkan ===");
    m_Queue.wait_idle();
    destroy_framebuffers();
    reset_command_pool();
    vkDestroyRenderPass(m_Device, m_RenderPass, VK_NULL_HANDLE);
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

VkResult VulkanContext::reset_command_buffer(VkCommandBuffer command_buffer)
{
    VkResult result = vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    return result;
}

void VulkanContext::submit(const std::vector<VkCommandBuffer> &command_buffers)
{
    m_Queue.submit_async(command_buffers);
}

uint32_t VulkanContext::get_current_image_index()
{
    return m_ImageIndex;
}

void VulkanContext::destroy_framebuffers()
{
    for (const auto framebuffer : m_Framebuffers)
        vkDestroyFramebuffer(m_Device, framebuffer, VK_NULL_HANDLE);
    m_Framebuffers.clear();
}

void VulkanContext::reset_command_pool() const
{
    m_Queue.wait_idle();

    VkResult result = vkResetCommandPool(m_Device, m_CommandPool, 0);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to reset command pool");
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

void VulkanContext::create_framebuffers()
{
    const auto extent = m_SwapChain.get_extent();
    const u32 image_count = m_SwapChain.get_image_count();
    m_Framebuffers.resize(image_count);

    for (u32 i = 0; i < image_count; i++)
    {
        VkImageView attachments[] = { m_SwapChain.get_image_view(i) };
        VkFramebufferCreateInfo framebuffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = VK_NULL_HANDLE,
            .renderPass = m_RenderPass,
            .attachmentCount = static_cast<uint32_t>(std::size(attachments)),
            .pAttachments = attachments,
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(m_Device, &framebuffer_create_info, VK_NULL_HANDLE, &m_Framebuffers[i]);
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

std::optional<uint32_t> VulkanContext::begin_frame()
{
    m_Queue.wait_idle();
    VkResult result = m_SwapChain.acquire_next_image(&m_ImageIndex, m_Queue.get_semaphore());
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate_swap_chain();
        return std::nullopt;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("[Vulkan] Failed to acquire SwapChain image");
    }

    m_Queue.wait_and_reset_fences();

    return m_ImageIndex;
}

void VulkanContext::present()
{
    VkResult result = m_Queue.present(m_ImageIndex, m_SwapChain.get_handle());
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreate_swap_chain();
    }
}

VkFramebuffer VulkanContext::get_framebuffer(uint32_t image_index) const
{
    ASSERT(image_index < m_Framebuffers.size(), "[Vulkan] Framebuffer index out of range");
    return m_Framebuffers[image_index];
}