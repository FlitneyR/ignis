#version 450

layout (location = 0) out vec4 f_output;

layout (location = 0) in vec2 i_uv;

layout (set = 0, binding = 0) uniform sampler2D t_source;

layout (push_constant) uniform PassConfig {
    int operation;
    int sourceMipLevel;
    int direction;
    float clipping;
    float dispersion;
    float mixing;
} pass;

#define OPERATION_FILTER 0
#define OPERATION_BLUR 1
#define OPERATION_OVERLAY 2

#define DIRECTION_HORIZONTAL 0
#define DIRECTION_VERTICAL 1

vec2 directions[] = vec2[]( vec2(1, 0), vec2(0, 1) );

void filterPass();
void blurPass();
void overlayPass();

void main() {
    switch (pass.operation) {
        case OPERATION_FILTER:  filterPass();  break;
        case OPERATION_BLUR:    blurPass();    break;
        case OPERATION_OVERLAY: overlayPass(); break;
    }
}

void filterPass() {
    f_output = textureLod(t_source, i_uv, pass.sourceMipLevel);

    float luminance = length(f_output.rgb) - pass.clipping;

    f_output.rbg = max(normalize(f_output.rgb) * luminance, vec3(0));
    f_output.a = 1;
}

void blurPass() {
    vec2 texelSize = pass.dispersion / textureSize(t_source, pass.sourceMipLevel);

    for (float f = -1.0; f <= 1.0; f += 0.25) {
        vec2 uv = i_uv + f * directions[pass.direction] * texelSize;

        f_output += exp(-3 * f * f) * texture(t_source, uv);
    }

    f_output /= f_output.a;
}

void overlayPass() {
    f_output = textureLod(t_source, i_uv, pass.sourceMipLevel);
    f_output.a = pass.mixing;
}
