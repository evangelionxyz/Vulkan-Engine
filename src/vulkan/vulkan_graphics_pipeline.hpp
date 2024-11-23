// Copyright 2024, Evangelion Manuhutu

#ifndef VULKAN_GRAPHICS_PIPELINE_HPP
#define VULKAN_GRAPHICS_PIPELINE_HPP

#include "vulkan_shader.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

struct PipelineCreateInfo
{
    VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
    VkPipelineRasterizationStateCreateInfo rasterization_info = {};
    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    VkPipelineColorBlendStateCreateInfo color_blend_info = {};
    VkPipelineLayoutCreateInfo layout_info = {};
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkViewport viewport = {};
    VkRect2D scissor = {};
    Ref<VulkanShader> shader;
};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline() = default;
    VulkanGraphicsPipeline(VkDevice device, VkAllocationCallbacks *allocator);

    void create(const PipelineCreateInfo &create_info);
    void destroy();

    void begin_render_pass(VkCommandBuffer command_buffer, VkFramebuffer framebuffer, VkClearValue clear_color, u32 width, u32 height);
    void end_render_pass(VkCommandBuffer command_buffer);

    void resize(u32 width, u32 height);

    VkPipeline get_vk_pipeline() const { return m_Pipeline; }
    VkPipelineLayout get_vk_pipeline_layout() const { return m_PipelineLayout; }
    VkRenderPass get_vk_render_pass() const { return m_RenderPass; }

    VkViewport get_vk_viewport() const { return m_Viewport; }
    VkRect2D get_vk_scissor() const { return m_Scissor; }

private:
    VkDevice m_Device;
    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkRenderPass m_RenderPass;

    VkViewport m_Viewport;
    VkRect2D m_Scissor;

    Ref<VulkanShader> m_Shader;

    VkAllocationCallbacks *m_Allocator;
};

#endif