#version 450

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 color;

void main() {
    color = v_color;
    gl_Position = v_position;
}
