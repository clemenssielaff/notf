#version 320 es

precision mediump float;

in VertexData {
    vec2 position;
    vec2 tex_coord;
    vec2 line_start;
    vec2 line_end;
    mediump flat int patch_type;
} v_in;

layout(location=0) out vec4 f_color;

// patch types
const int CONVEX    = 1;
const int CONCAVE   = 2;
const int STROKE    = 3;
const int TEXT      = 4;
const int JOINT     = 31;
const int START_CAP = 32;
const int END_CAP   = 33;

const float ZERO = 0.0f;
const float ONE = 1.0f;

uniform sampler2D font_texture;

void main() {

    if(v_in.patch_type == TEXT){
        f_color = vec4(ONE, ONE, ONE, texture(font_texture, v_in.tex_coord).r);
    }
    else {
        f_color = vec4(ONE, ONE, ONE, mix(ZERO, ONE, v_in.tex_coord.y));
    }
}
