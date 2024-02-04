#version 450

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec4 v_color;
layout (location = 2) in mat4 v_transform;

layout (location = 0) out vec4 color;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

void main() {
    color = v_color;
    gl_Position = camera.projection * camera.view * v_transform * v_position;
}
