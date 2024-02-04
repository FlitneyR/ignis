#version 450

layout (location = 0) in vec4 color;

layout (location = 0) out vec4 f_color;

layout (set = 1, binding = 0) uniform sampler2D u_texture;

void main() {
    f_color = texture(u_texture, color.xy);
}
