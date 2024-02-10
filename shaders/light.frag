#version 450

#include "pbr.glsl"

layout (location = 0) out vec4 f_emissive;

layout (location = 0) in vec2 i_uv;

layout (set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;

layout (set = 1, binding = 0) uniform sampler2D t_depth;
layout (set = 1, binding = 1) uniform sampler2D t_albedo;
layout (set = 1, binding = 2) uniform sampler2D t_normal;
layout (set = 1, binding = 4) uniform sampler2D t_aoMetalRough;

layout (push_constant) uniform Light {
    int   type;
    vec3  position;
    vec3  direction;
    vec4  color;
} light;

// struct Light {
//     int  type;
//     vec3 position;
//     vec3 direction;
//     vec3 color;
// };

// layout (set = 2, binding = 0) uniform Lights {
//     int count;
//     Light lights[];
// } lights;

#define AMBIENT_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2
#define DIRECITONAL_LIGHT 3

void main() {
    vec2 uv = i_uv;

    float depth = texture(t_depth, uv).r;
    vec4  csPosition = vec4(2.0 * uv - 1.0, depth, 1.0);
    vec4  wsPosition = inverse(camera.projection * camera.view) * csPosition;
    vec3  position = wsPosition.xyz / wsPosition.w;

    vec3  camPos = inverse(camera.view)[3].xyz;
    vec3  viewDir = camPos - position;

    vec3  albedo = texture(t_albedo, uv).rgb;
    vec3  normal = texture(t_normal, uv).rgb;
    vec3  aoMetalRough = texture(t_aoMetalRough, uv).rgb;
    float ao = aoMetalRough.r;
    float metallic = aoMetalRough.g;
    float roughness = aoMetalRough.b;

    vec3 lightDir;
    vec3 radiance = light.color.rgb * light.color.a;

    if (light.type == DIRECITONAL_LIGHT) {
        lightDir = normalize(-light.direction);
    }

    if (light.type == POINT_LIGHT || light.type == SPOT_LIGHT) {
        lightDir = position - light.position;
        float lightDist = length(lightDir);
        radiance /= lightDist * lightDist;
        lightDir = lightDir / lightDist;
    }

    if (light.type == AMBIENT_LIGHT)
        f_emissive.rgb += albedo * ao * radiance;// * pow(1.0 - metallic);
    else
        f_emissive.rgb += cookTorranceBRDF(albedo, normal, viewDir, radiance, -lightDir, metallic, roughness, ao);

    f_emissive.a = 1.0;
}
