// Copyright (c) 2025, Evangelion Manuhutu

#ifndef VULKAN_GRAPHICS_PIPELINE_HPP
#define VULKAN_GRAPHICS_PIPELINE_HPP

#include "vulkan_shader.hpp"

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

struct VulkanGraphicsPipelineInfo
{
    // Store actual data, not pointers
    VkVertexInputBindingDescription binding_description;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    VkPipelineLayout layout;

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
    VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    VkColorComponentFlags color_write_mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkBlendFactor src_color_blend_factor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dst_color_blend_factor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp color_blend_op = VK_BLEND_OP_ADD;
    VkBlendFactor src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alpha_blend_op = VK_BLEND_OP_ADD;
    VkExtent2D extent;
    float line_width = 1.0f;
    bool depth_test = true;
    bool depth_write = false;
    bool depth_bias = false;
    bool blending = false;
    bool stencil_test = false;
};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(VkRenderPass render_pass);

    VulkanGraphicsPipeline &add_shader(const Ref<VulkanShader> &shader);
    void build(const VulkanGraphicsPipelineInfo &info);

    void begin(VkCommandBuffer command_buffer, VkFramebuffer framebuffer, const VkClearValue &clear_color, const VkExtent2D &extent) const;
    static void end(VkCommandBuffer command_buffer);

    void destroy();

    VkRenderPass get_render_pass() const { return m_RenderPass; }
    VkPipeline get_handle() const { return m_Handle; }
    VkPipelineLayout get_layout() const { return m_Layout; }

private:
    VkPipeline m_Handle;
    VkPipelineLayout m_Layout;
    VkRenderPass m_RenderPass;
    std::vector<Ref<VulkanShader>> m_Shaders;
};

#endif