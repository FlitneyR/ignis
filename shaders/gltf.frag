#version 450

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

const vec3 lightCol =           vec3(10.0);
const vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
const vec3 ambient  =           vec3(0.05);

void main() {
    float ao           = texture(t_ao, i_uv).r;
    vec2  metalRough   = texture(t_metalRough, i_uv).gb;
    vec4  albedo       = texture(t_albedo, i_uv) * material.baseColorFactor;
    vec3  normalTSpace = texture(t_normal, i_uv).rgb - 0.5;
    vec3  emissive     = texture(t_emissive, i_uv).rgb * material.emissiveFactor;
    float metallic     = metalRough.x * material.metallicFactor;
    float roughness    = metalRough.y * material.roughnessFactor;

    vec3  viewDir = normalize(i_campos - i_position.xyz);
    vec3  halfway = normalize(viewDir + lightDir);
    vec3  normal  = normalize(mat3(i_tangent, i_bitangent, i_normal) * normalTSpace);

    vec3  diffuse  =    (lightCol * max(dot(lightDir, normal), 0) * roughness + ambient)   * ao * albedo.rgb;
    vec3  specular = lightCol * pow(max(dot(halfway,  normal), 0), (20.0 * (2.0 - roughness))) * mix(vec3(0.05), albedo.rgb, metallic);
    vec3  lighting = specular + diffuse;

    f_color.rgb = lighting + emissive;
    f_color.a = albedo.a;
}
