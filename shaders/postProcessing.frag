#version 450

layout (location = 0) out vec4 f_color;

layout (location = 0) in vec2 i_uv;

layout (set = 0, binding = 3) uniform sampler2D t_emissive;

vec3 aces(vec3 x);
float aces(float x);

void main() {
    vec2 uv = i_uv;

    vec4 emissive = texture(t_emissive, uv);
    // emissive.rgb /= emissive.a;
    emissive.rgb /= 1.0 + emissive.rgb;

    f_color = emissive;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float aces(float x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
