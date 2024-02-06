#version 450

layout (location = 0) in vec4 v_position;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_normal;
layout (location = 3) in vec3 v_tangent;
layout (location = 4) in mat4 v_transform;

layout (location = 0) out vec2 o_uv;
layout (location = 1) out vec4 o_position;
layout (location = 2) out vec3 o_normal;
layout (location = 3) out vec3 o_tangent;
layout (location = 4) out vec3 o_bitangent;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

void main() {
    o_uv = v_uv;
    o_position = v_transform * v_position;
    o_normal = normalize(mat3(v_transform) * v_normal);
    o_tangent = normalize(mat3(v_transform) * v_tangent);
    o_bitangent = cross(o_normal, o_tangent);
    gl_Position = camera.projection * camera.view * o_position;
}
