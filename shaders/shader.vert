#version 450

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec4 v_color;
layout (location = 2) in mat4 v_transform;

layout (location = 0) out vec4 color;

layout (push_constant) uniform PushConstant {
    mat4 view;
    mat4 projection;
} pc;

void main() {
    color = v_color;
    gl_Position = pc.projection * pc.view * v_transform * v_position;
}
