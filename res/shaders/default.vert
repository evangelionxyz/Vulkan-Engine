#version 450 core

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
} pc;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;

layout (location = 0) out vec3 v_color;

void main()
{
    gl_Position = pc.viewProjection * vec4(a_position, 1.0);
    v_color = a_color;
}
