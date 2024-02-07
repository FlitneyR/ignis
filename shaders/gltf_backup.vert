#version 450

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_normal;
layout (location = 3) in mat4 v_transform;

layout (location = 0) out vec3 o_campos;
layout (location = 1) out vec2 o_uv;
layout (location = 2) out vec4 o_position;
layout (location = 3) out vec3 o_normal;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

void main() {
    o_campos = inverse(camera.view)[3].xyz;
    o_uv = v_uv;
    o_position = v_transform * v_position;
    o_normal = normalize(mat3(v_transform) * v_normal);
    gl_Position = camera.projection * camera.view * o_position;
}
