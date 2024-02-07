#version 450

#include "pbr.glsl"

layout (location = 0) out vec4 f_color;

layout (location = 0) in vec3 i_campos;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec4 i_position;
layout (location = 3) in vec3 i_normal;
layout (location = 4) in vec3 i_tangent;
layout (location = 5) in vec3 i_bitangent;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

layout (set = 1, binding = 0) uniform sampler2D t_albedo;
layout (set = 1, binding = 1) uniform sampler2D t_metalRough;
layout (set = 1, binding = 2) uniform sampler2D t_emissive;
layout (set = 1, binding = 3) uniform sampler2D t_ao;
layout (set = 1, binding = 4) uniform sampler2D t_normal;

layout ( push_constant ) uniform Material {
    vec3  emissiveFactor;
    vec4  baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
} material;

const vec3 lightCol =           vec3(5.0);
const vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
const vec3 ambient  =           vec3(0.05);

void main() {
    float ao           = texture(t_ao, i_uv).r;
    vec2  metalRough   = texture(t_metalRough, i_uv).gb;
    vec4  albedo       = texture(t_albedo, i_uv) * material.baseColorFactor;
    vec3  normalTSpace = texture(t_normal, i_uv).rgb - 0.5;
    vec3  emissive     = texture(t_emissive, i_uv).rgb * material.emissiveFactor;
    float metallic     = metalRough.g * material.metallicFactor;
    float roughness    = metalRough.r * material.roughnessFactor;
    vec3  viewDir      = normalize(i_campos - i_position.xyz);
    vec3  normal       = normalize(mat3(i_tangent, i_bitangent, i_normal) * normalTSpace);

    f_color.rgb += cookTorranceBRDF(albedo.rgb, normal, viewDir, lightCol, lightDir, metallic, roughness, ao);
    f_color.rgb += albedo.rgb * ambient * ao;
    f_color.rgb += emissive;

    f_color.rgb /= f_color.rgb + 1.0;

    f_color.a = albedo.a;
}
