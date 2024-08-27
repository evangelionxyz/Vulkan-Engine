// Copyright 2024, Evangelion Manuhutu

#include "vulkan_context.h"

#include <cstdio>
#include <iterator>
#include "vulkan_wrapper.h"
#include "core/assert.h"

VulkanContext::VulkanContext(GLFWwindow* window)
    : m_Window(window)
{
    create_instance();
    //create_debug_callback();

    create_window_surface();
    m_PhysicalDevice = VulkanPhysicalDevice(m_Instance, m_Surface);
    m_QueueFamily = m_PhysicalDevice.select_device(VK_QUEUE_GRAPHICS_BIT, true);
    create_device();
    create_swapchain();
    create_command_pool();

    m_Queue = VulkanQueue(m_LogicalDevice, m_Swapchain, m_Allocator, m_QueueFamily, 0);
}

VulkanContext::~VulkanContext()
{
    // destroy command  pool
    vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, m_Allocator);
    LOG_INFO("[Vulkan Context] Command pool destroyed");

    // destroy semaphores
    m_Queue.destroy_semaphores();
    LOG_INFO("[Vulkan Context] Semaphores destroyed");

    // destroy image view
    for (const auto image_view : m_SwapchainImageViews)
        vkDestroyImageView(m_LogicalDevice, image_view, m_Allocator);
    LOG_INFO("[Vulkan Context] Image views destroyed");

    // destroy swapchain
    vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, m_Allocator);
    LOG_INFO("[Vulkan Context] Swapchain destroyed");

    // destroy surface
    vkDestroySurfaceKHR(m_Instance, m_Surface, m_Allocator);
    LOG_INFO("[Vulkan Context] Window surface destroyed");

    // destroy debug messenger
    PFN_vkDestroyDebugUtilsMessengerEXT vk_destroy_debug_utils_messenger = VK_NULL_HANDLE;
    vk_destroy_debug_utils_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
    ASSERT(vk_destroy_debug_utils_messenger, "[Vulkan Context] Cannot find address of vkDestroyDebugUtilsMessengerEXT");
    vk_destroy_debug_utils_messenger(m_Instance, m_DebugMessenger, m_Allocator);
    LOG_INFO("[Vulkan Context] Debug messenger destroyed");

    // destroy device
    vkDestroyDevice(m_LogicalDevice, m_Allocator);
    LOG_INFO("[Vulkan Context] Logical device destroyed");

    // destroy instance
    vkDestroyInstance(m_Instance, m_Allocator);
    LOG_INFO("[Vulkan Context] Instance destroyed");
}

std::vector<VkFramebuffer> VulkanContext::create_framebuffers(VkRenderPass render_pass)
{
    std::vector<VkFramebuffer> framebuffers;
    framebuffers.resize(m_SwapchainImages.size());
    i32 width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
    for (size_t i = 0; i < m_SwapchainImages.size(); i++)
    {
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass      = render_pass;
        framebuffer_create_info.width           = width;
        framebuffer_create_info.height          = height;
        framebuffer_create_info.layers          = 1;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments    = &m_SwapchainImageViews[i];
        VK_ERROR_CHECK(vkCreateFramebuffer(m_LogicalDevice, &framebuffer_create_info, m_Allocator, &framebuffers[i]),
            "[vkCreateFramebuffer] Failed to create framebuffer");
    }
    LOG_INFO("[Vulkan Context] Framebuffers created");
    return framebuffers;
}

VkRenderPass VulkanContext::create_render_pass()
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format         = m_SwapchainSurfaceFormat.format;
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
    subpass.pInputAttachments = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &color_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VK_ERROR_CHECK(vkCreateRenderPass(m_LogicalDevice, &render_pass_info, m_Allocator, &render_pass),
                   "[vkCreateRenderPass] Failed to create render pass");

    LOG_INFO("[Vulkan Context] Render pass created");
    return render_pass;
}

void VulkanContext::destroy_framebuffers(const std::vector<VkFramebuffer> &framebuffers)
{
    for (const auto framebuffer : framebuffers)
        vkDestroyFramebuffer(m_LogicalDevice, framebuffer, m_Allocator);
}

VkDevice VulkanContext::get_logical_device()
{
    return m_LogicalDevice;
}

i32 VulkanContext::get_image_count() const
{
    return static_cast<i32>(m_SwapchainImages.size());
}

const VkImage& VulkanContext::get_image(i32 index)
{
    return m_SwapchainImages[index];
}

VulkanQueue* VulkanContext::get_queue()
{
    return &m_Queue;
}

u32 VulkanContext::get_queue_family() const
{
    return m_QueueFamily;
}

void VulkanContext::create_command_buffers(u32 count, VkCommandBuffer* command_buffers)
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandBufferCount = count;
    alloc_info.commandPool        = m_CommandPool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkResult result = vkAllocateCommandBuffers(m_LogicalDevice, &alloc_info, command_buffers);
    VK_ERROR_CHECK(result, "[vkAllocatedCommandBuffers] Failed to create command buffers");
    LOG_INFO("[Vulkan Context] Command buffers created");
}

void VulkanContext::free_command_buffers(u32 count, const VkCommandBuffer* command_buffers)
{
    m_Queue.wait_idle();
    vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, count, command_buffers);
}

VkAllocationCallbacks* VulkanContext::get_allocator()
{
    return m_Allocator;
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

    VkResult res = vkCreateInstance(&create_info, m_Allocator, &m_Instance);
    VK_ERROR_CHECK(res, "[vkCreateInstance] Failed to create instance");
    LOG_INFO("[Vulkan Context] Vulkan instance created");
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
    vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
    ASSERT(vkCreateDebugUtilsMessenger, "[Vulkan Context] Cannot find address of vkCreateDebugUtilsMessenger");

    VkResult result = vkCreateDebugUtilsMessenger(m_Instance, &msg_create_info, m_Allocator, &m_DebugMessenger);
    VK_ERROR_CHECK(result, "[vkCreateDebugUtilsMessengerEXT] Failed to create debug messenger");
    LOG_INFO("[Vulkan Context] Debug utils messenger created");
}

void VulkanContext::create_window_surface()
{
    VkResult res = glfwCreateWindowSurface(m_Instance, m_Window, m_Allocator, &m_Surface);
    VK_ERROR_CHECK(res, "[glfwCreateWindowSurface] Failed to create window surface");
    LOG_INFO("[Vulkan Context] Window surface created");
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
    {
        LOG_ERROR("[Vulkan Context] Geometry shader is not supported");
    }

    if (m_PhysicalDevice.get_selected_device().Features.tessellationShader == VK_FALSE)
    {
        LOG_ERROR("[Vulkan Context] Tesselation shader is not supported");
    }

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
    VK_ERROR_CHECK(result, "[vkCreateDevice] Failed to create logical device");
    LOG_INFO("[Vulkan Context] Logical device created\n");
}

void VulkanContext::create_swapchain()
{
    i32 width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
    VkSurfaceCapabilitiesKHR capabilities = m_PhysicalDevice.get_selected_device().SurfaceCapabilities;
    VkExtent2D swapchain_extent = { static_cast<u32>(width), static_cast<u32>(height) };
    capabilities.currentExtent.width = std::clamp(swapchain_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    capabilities.currentExtent.height = std::clamp(swapchain_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    u32 image_count = vk_choose_images_count(capabilities);

    const std::vector<VkPresentModeKHR> &present_modes = m_PhysicalDevice.get_selected_device().PresentModes;
    const VkPresentModeKHR present_mode = vk_choose_present_mode(present_modes);

    m_SwapchainSurfaceFormat = vk_choose_surface_format(m_PhysicalDevice.get_selected_device().SurfaceFormats);

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface               = m_Surface;
    swapchain_create_info.minImageCount         = image_count;
    swapchain_create_info.imageFormat           = m_SwapchainSurfaceFormat.format;
    swapchain_create_info.imageColorSpace       = m_SwapchainSurfaceFormat.colorSpace;
    swapchain_create_info.imageExtent           = capabilities.currentExtent;
    swapchain_create_info.imageArrayLayers      = 1;
    swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices   = &m_QueueFamily;
    swapchain_create_info.preTransform          = capabilities.currentTransform;
    swapchain_create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode           = present_mode;
    swapchain_create_info.clipped               = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(m_LogicalDevice, &swapchain_create_info, m_Allocator, &m_Swapchain);
    VK_ERROR_CHECK(result, "[vkCreateSwapchainKHR] Failed to create swapchain");
    LOG_INFO("[Vulkan Context] Swapchain created\n");

    // create swapchain images
    u32 swapchain_image_count = 0;
    result = vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &swapchain_image_count, nullptr);
    VK_ERROR_CHECK(result, "[vkGetSwapchainImagesKHR] Failed to get swapchain count");
    ASSERT(image_count <= swapchain_image_count, "[Vulkan Context] Swapchain image count exceeds maximum number of images");

    LOG_INFO("[Vulkan Context] Requested {0} images, created {1} images", swapchain_image_count, swapchain_image_count);

    m_SwapchainImages.resize(swapchain_image_count);
    m_SwapchainImageViews.resize(swapchain_image_count);

    result = vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &swapchain_image_count, m_SwapchainImages.data());
    VK_ERROR_CHECK(result, "[vkGetSwapchainImagesKHR] Failed to get swapchain images");

    i32 layer_count = 1;
    i32 mip_levels = 1;
    for (u32 i = 0; i < swapchain_image_count; ++i)
    {
        m_SwapchainImageViews[i] = vk_create_image_view(m_LogicalDevice, m_SwapchainImages[i], m_Allocator,
            m_SwapchainSurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, layer_count, mip_levels);
    }

}

void VulkanContext::create_command_pool()
{
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.queueFamilyIndex = m_QueueFamily;

    VkResult result = vkCreateCommandPool(m_LogicalDevice, &pool_create_info, m_Allocator, &m_CommandPool);
    VK_ERROR_CHECK(result, "[vkCreateCommandPool] Failed to create command pool");

    LOG_INFO("[Vulkan Context] Command buffer created\n");
}
