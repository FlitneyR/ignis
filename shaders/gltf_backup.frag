#version 450

layout (location = 0) out vec4 f_color;

layout (location = 0) in vec3 i_campos;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec4 i_position;
layout (location = 3) in vec3 i_normal;

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

const vec3 lightCol =           vec3(1.0);
const vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
const vec3 ambient  =           vec3(0.01);

void main() {
    float ao         = texture(t_ao, i_uv).r;
    vec2  metalRough = texture(t_metalRough, i_uv).gb;
    vec4  albedo     = texture(t_albedo, i_uv) * material.baseColorFactor;
    vec3  emissive   = texture(t_emissive, i_uv).rgb * material.emissiveFactor;

    vec3  viewDir = normalize(i_position.xyz - i_campos);
    vec3  halfway = normalize(viewDir + lightDir);
    vec3  normal  = i_normal;

    float metallic  = metalRough.x * material.metallicFactor;
    float roughness = metalRough.y * material.roughnessFactor;

    vec3  diffuse  =     lightCol * max(dot(lightDir, normal), 0) * ao;
    vec3  specular = pow(lightCol * max(dot(halfway,  normal), 0), vec3(20.0));

    vec3  lighting = diffuse + ambient;

    // f_color.rgb = i_tangent;
    f_color.rgb = halfway;
    // f_color.rgb = albedo.rgb;

    // f_color.rgb = normal;
    f_color.a = albedo.a;
}
