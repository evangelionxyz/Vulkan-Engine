#version 450 core

// layout(push_constant) uniform PushConstants {
//     mat4 viewProjection;
// } pc;

layout(binding = 0) uniform UniformBufferObject {
    mat4 viewProjection;
    mat4 transform;
} ubo;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

layout(location = 0) out vec3 v_color;

void main()
{
    gl_Position = ubo.viewProjection * ubo.transform * vec4(a_position, 1.0);
    v_color = a_color;
}
