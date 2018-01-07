#version 320 es

precision mediump float;

in VertexData {
    vec2 position;
    vec2 tex_coord;
} v_in;

layout(location=0) out vec4 f_color;

const float ZERO = 0.0f;
const float ONE = 1.0f;

void main() {
//    f_color = vec4(v_in.tex_coord.x, ZERO, ZERO, smoothstep(ZERO, ONE, v_in.tex_coord.y));
    f_color = vec4(ONE, ONE, ONE, smoothstep(ZERO, ONE, v_in.tex_coord.y));
//    f_color = vec4(ONE, ONE, ONE, 0.40);
}
