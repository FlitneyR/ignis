#version 450

vec4[] screenPositions = vec4[](
    vec4(-1, -1, 0.5, 1),
    vec4(-1,  3, 0.5, 1),
    vec4( 3, -1, 0.5, 1)
);

layout (location = 0) out vec2 o_uv;

void main() {
    gl_Position = screenPositions[gl_VertexIndex.x];
    o_uv = 0.5 * gl_Position.xy + 0.5;
}
