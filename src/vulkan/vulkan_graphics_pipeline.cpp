// Copyright 2024, Evangelion Manuhutu

#include "vulkan_graphics_pipeline.h"
#include "vulkan_wrapper.h"

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkDevice device, VkRenderPass render_pass, VkAllocationCallbacks *allocator)
    : m_Device(device), m_RenderPass(render_pass), m_Allocator(allocator), m_Pipeline(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE)
{
}

void VulkanGraphicsPipeline::create(
        const std::vector<VkPipelineShaderStageCreateInfo> &shader_stages,
        const VkPipelineVertexInputStateCreateInfo &vertex_input_info,
        const VkPipelineInputAssemblyStateCreateInfo &input_assembly_info,
        const VkViewport &viewport,
        const VkRect2D &scissor,
        const VkPipelineRasterizationStateCreateInfo &rasterization_info,
        const VkPipelineMultisampleStateCreateInfo &multisample_info,
        const VkPipelineColorBlendStateCreateInfo &color_blend_info,
        const VkPipelineLayoutCreateInfo &layout_info)
{
    // create pipeline layout
    VkResult result = vkCreatePipelineLayout(m_Device, &layout_info, nullptr, &m_PipelineLayout);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create pipeline layout");

    // viewport state
    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    // dynamic states
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamicStateInfo.pDynamicStates = dynamic_states.data();

    // graphics pipeline creation info
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
    pipelineInfo.pStages = shader_stages.data();
    pipelineInfo.pVertexInputState = &vertex_input_info;
    pipelineInfo.pInputAssemblyState = &input_assembly_info;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterization_info;
    pipelineInfo.pMultisampleState = &multisample_info;
    pipelineInfo.pColorBlendState = &color_blend_info;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, 
        &pipelineInfo, m_Allocator, &m_Pipeline);

    VK_ERROR_CHECK(result, "[Vulkan] Failed to create graphics pipeline");
}

void VulkanGraphicsPipeline::destroy()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device, m_Pipeline, m_Allocator);
    } 

    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, m_Allocator);
    }
}