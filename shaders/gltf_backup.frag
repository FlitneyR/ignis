#version 450

#include "pbr.glsl"

layout (location = 0) out vec4 f_albedo;
layout (location = 1) out vec4 f_normal;
layout (location = 2) out vec4 f_emissive;
layout (location = 3) out vec4 f_aoRoughMetal;

layout (location = 0) in vec2 i_uv;
layout (location = 1) in vec4 i_position;
layout (location = 2) in vec3 i_normal;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

layout (set = 1, binding = 0) uniform sampler2D t_albedo;
layout (set = 1, binding = 1) uniform sampler2D t_metalRough;
layout (set = 1, binding = 2) uniform sampler2D t_emissive;
layout (set = 1, binding = 3) uniform sampler2D t_ao;

layout ( push_constant ) uniform Material {
    vec3  emissiveFactor;
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
} material;

void main() {
    vec4 albedo = texture(t_albedo, i_uv) * material.baseColorFactor;
    if (albedo.a < 0.5) discard;

    float ao         = texture(t_ao, i_uv).r;
    vec2  metalRough = texture(t_metalRough, i_uv).gb;
    vec3  emissive   = texture(t_emissive, i_uv).rgb * material.emissiveFactor;

    vec3  normal     = i_normal;
    float metallic   = metalRough.g * material.metallicFactor;
    float roughness  = metalRough.r * material.roughnessFactor;

    f_albedo = albedo;
    f_normal = vec4(normal, 1.0);
    f_emissive = vec4(emissive, 1.0);
    f_aoRoughMetal = vec4(ao, metallic, roughness, 1.0);
}
