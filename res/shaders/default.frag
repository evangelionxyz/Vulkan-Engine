#version 450 core

layout (location = 0) out vec4 o_color;
layout (location = 0) in vec3 v_color;

void main()
{
    oColor = vec4(v_color, 1.0);
}