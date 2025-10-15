// Copyright (c) 2025 Evangelion Manuhutu

#include "vulkan_graphics_pipeline.hpp"
#include "vulkan_wrapper.hpp"

#include <algorithm>

#include "vulkan_context.hpp"

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkRenderPass render_pass)
    : m_RenderPass(render_pass), m_Handle(VK_NULL_HANDLE), m_Layout(VK_NULL_HANDLE)
{
}

void VulkanGraphicsPipeline::begin(const VkCommandBuffer command_buffer, const VkFramebuffer framebuffer, const VkClearValue &clear_color, const VkExtent2D &extent) const
{
    const VkRect2D render_area = {
        .offset = { 0, 0 },
        .extent = extent
    };

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = VK_NULL_HANDLE,
        .renderPass = m_RenderPass,
        .framebuffer = framebuffer,
        .renderArea = render_area,
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE); // render pass
}

void VulkanGraphicsPipeline::end(const VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}

void VulkanGraphicsPipeline::destroy()
{
    auto device = VulkanContext::get()->get_device();
    
    if (m_Handle != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, m_Handle, VK_NULL_HANDLE);
        m_Handle = VK_NULL_HANDLE;
    }

    if (m_Layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, m_Layout, VK_NULL_HANDLE);
        m_Layout = VK_NULL_HANDLE;
    }

    m_Shaders.clear();
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::add_shader(const Ref<VulkanShader>& shader)
{
    m_Shaders.push_back(shader);
    return *this;
}

void VulkanGraphicsPipeline::build(const VulkanGraphicsPipelineInfo& info)
{
    auto device = VulkanContext::get()->get_device();

    // Store the pipeline layout
    m_Layout = info.layout;

    VkPipelineRasterizationStateCreateInfo rasterization_info = {};
    rasterization_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_info.depthClampEnable        = VK_FALSE;
    rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_info.polygonMode             = info.polygon_mode;
    rasterization_info.lineWidth               = 1.0f;
    rasterization_info.cullMode                = info.cull_mode;
    rasterization_info.frontFace               = info.front_face;
    rasterization_info.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.sampleShadingEnable  = VK_FALSE;
    multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = info.color_write_mask;
    color_blend_attachment.blendEnable    = info.blending;

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

    // Viewport state with dynamic viewport and scissor (no static values needed)
    VkPipelineViewportStateCreateInfo viewport_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = nullptr,  // Will be set dynamically
        .scissorCount = 1,
        .pScissors = nullptr    // Will be set dynamically
    };

    // Dynamic states
    VkDynamicState dynamic_states[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(std::size(dynamic_states)),
        .pDynamicStates = dynamic_states
    };

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages {};
    for (auto& shader : m_Shaders)
    {
        shader_stages.push_back(shader->get_stage());
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = info.topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Build vertex input state from stored data
    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &info.binding_description,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(info.attribute_descriptions.size()),
        .pVertexAttributeDescriptions = info.attribute_descriptions.data()
    };

    // Graphics pipeline creation info
    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_create_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = info.layout,
        .renderPass = m_RenderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    // Create the new pipeline
    VkResult result = vkCreateGraphicsPipelines(device,
        VK_NULL_HANDLE, 1,
        &pipeline_create_info, VK_NULL_HANDLE, &m_Handle);

    VK_ERROR_CHECK(result,"[Vulkan] Failed to recreate graphics pipeline");
}
