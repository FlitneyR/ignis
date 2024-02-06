#version 450

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in mat4 v_transform;

layout (location = 0) out vec2 uv;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

void main() {
    uv = v_uv;
    gl_Position = camera.projection * camera.view * v_transform * v_position;
}
