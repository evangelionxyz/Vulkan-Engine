// Copyright 2025, Evangelion Manuhutu

#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <vulkan/vulkan.h>
#include <array>

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription get_vk_binding_desc()
    {
        VkVertexInputBindingDescription binding_desc = {};
        binding_desc.binding   = 0;
        binding_desc.stride    = sizeof(Vertex);
        binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding_desc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> get_vk_attribute_desc()
    {
        std::array<VkVertexInputAttributeDescription, 2> attr_desc = {};
        // position attribute
        attr_desc[0].binding  = 0;
        attr_desc[0].location = 0;
        attr_desc[0].format   = VK_FORMAT_R32G32_SFLOAT; // vector 2
        attr_desc[0].offset   = offsetof(Vertex, position);

        // color attribute
        attr_desc[1].binding  = 0;
        attr_desc[1].location = 1;
        attr_desc[1].format   = VK_FORMAT_R32G32B32_SFLOAT; // vector 3
        attr_desc[1].offset   = offsetof(Vertex, color);

        return attr_desc;
    }
};


#endif