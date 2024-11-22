// Copyright 2024, Evangelion Manuhutu

#ifndef VULKAN_GRAPHICS_PIPELINE_HPP
#define VULKAN_GRAPHICS_PIPELINE_HPP

#include <vulkan/vulkan.h>

#include <vector>
#include <stdexcept>

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline() = default;
    VulkanGraphicsPipeline(VkDevice device, VkRenderPass render_pass, VkAllocationCallbacks *allocator);

    void create(
        const std::vector<VkPipelineShaderStageCreateInfo> &shader_stages,
        const VkPipelineVertexInputStateCreateInfo &vertex_input_info,
        const VkPipelineInputAssemblyStateCreateInfo &input_assembly_info,
        const VkViewport &viewport,
        const VkRect2D &scissor,
        const VkPipelineRasterizationStateCreateInfo &rasterization_info,
        const VkPipelineMultisampleStateCreateInfo &multisample_info,
        const VkPipelineColorBlendStateCreateInfo &color_blend_info,
        const VkPipelineLayoutCreateInfo &layout_info
    );

    void destroy();

    VkPipeline get_vk_pipeline() const { return m_Pipeline; }
    VkPipelineLayout get_vk_pipeline_layout() const { return m_PipelineLayout; }
private:
    VkDevice m_Device;
    VkRenderPass m_RenderPass;
    
    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    
    VkAllocationCallbacks *m_Allocator;
};

#endif