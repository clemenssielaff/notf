#version 320 es

precision mediump float;

in VertexData {
    highp vec2 position;
    vec3 patch_distance;
} v_in;

layout(location=0) out vec4 f_color;

const float ZERO = 0.0f;
const float ONE = 1.0f;

void main() {
    f_color = vec4((v_in.position / 800.f), ZERO, ONE);
}
