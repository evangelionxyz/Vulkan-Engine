// Copyright 2024, Evangelion Manuhutu

#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_wrapper.hpp"

#include <algorithm>

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkDevice device, VkAllocationCallbacks *allocator)
    : m_Device(device), m_Allocator(allocator), m_Pipeline(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE)
{
}

void VulkanGraphicsPipeline::create(const PipelineCreateInfo &create_info)
{
    // create pipeline layout
    VkResult result = vkCreatePipelineLayout(m_Device, &create_info.layout_info, nullptr, &m_PipelineLayout);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to create pipeline layout");

    m_RenderPass = create_info.render_pass;

    m_Viewport = create_info.viewport;
    m_Scissor = create_info.scissor;
    m_Shader = create_info.shader;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_create_info = {};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.pViewports = &m_Viewport;
    viewport_create_info.scissorCount = 1;
    viewport_create_info.pScissors = &m_Scissor;

    // Dynamic states
    VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]);
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    // Graphics pipeline creation info
    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = static_cast<uint32_t>(create_info.shader->get_vk_stage_create_info().size());
    pipeline_create_info.pStages = create_info.shader->get_vk_stage_create_info().data();
    pipeline_create_info.pVertexInputState = &create_info.input_vertex_info;
    pipeline_create_info.pInputAssemblyState = &create_info.input_assembly_info;
    pipeline_create_info.pViewportState = &viewport_create_info;
    pipeline_create_info.pRasterizationState = &create_info.rasterization_info;
    pipeline_create_info.pMultisampleState = &create_info.multisample_info;
    pipeline_create_info.pColorBlendState = &create_info.color_blend_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = m_PipelineLayout;
    pipeline_create_info.renderPass = m_RenderPass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

    // Create the new pipeline
    result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipeline_create_info, m_Allocator, &m_Pipeline);
    VK_ERROR_CHECK(result, "[Vulkan] Failed to recreate graphics pipeline");
}

void VulkanGraphicsPipeline::begin_render_pass(VkCommandBuffer command_buffer, VkFramebuffer framebuffer, VkClearValue clear_color, u32 width, u32 height)
{
    const VkRect2D render_area = { { 0, 0 }, {  width, height} };
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = VK_NULL_HANDLE;
    render_pass_begin_info.renderPass = m_RenderPass;
    render_pass_begin_info.renderArea = render_area;
    render_pass_begin_info.framebuffer = framebuffer;
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_color;
    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE); // render pass
}

void VulkanGraphicsPipeline::end_render_pass(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}

void VulkanGraphicsPipeline::resize(u32 width, u32 height)
{
    m_Viewport.width = width;
    m_Viewport.height = height;
    m_Scissor.extent.width = width;
    m_Scissor.extent.height = height;
}

void VulkanGraphicsPipeline::destroy()
{
    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device, m_Pipeline, m_Allocator);
        VK_NULL_HANDLE;
    }

    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, m_Allocator);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    m_Shader.reset();
}
