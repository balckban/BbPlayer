#version 300 es
layout(location = 0) in vec4 attr_position;
layout(location = 1) in vec2 attr_uv;

uniform mat4   uni_mat;
out vec2   out_uv;

void main(void)
{
    out_uv = attr_uv;
    gl_Position = uni_mat* attr_position;
}
