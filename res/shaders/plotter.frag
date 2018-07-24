#version 320 es

precision mediump float;

in VertexData {
    mediump mat3 line_xform;
    mediump vec2 line_size;
    mediump vec2 position;
    mediump vec2 tex_coord;
    mediump flat int patch_type;
} v_in;

uniform sampler2D font_texture;

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

const float STEP = 1.0 / 16.0;

float sample_line()
{
    vec2 pixel = gl_FragCoord.xy - vec2(0.5, 0.5);
    float result = 0.;
    vec3 pos = vec3(1.);
    for(float x = 0.; x < 8.; ++x){
        pos.x = pixel.x + ((2. * x) + 1.) * STEP;
        for(float y = 0.; y < 8.; ++y){
            pos.y = pixel.y + ((2. * y) + 1.) * STEP;
            vec3 sample_pos = pos * v_in.line_xform;
            if(sample_pos.x >= 0. && sample_pos.x <= v_in.line_size.x &&
               sample_pos.y >= 0. && sample_pos.y <= v_in.line_size.y){
                result += 1.0 / 64.0;
            }
        }
    }
    return result;
}

void main() 
{
    if(v_in.patch_type == TEXT){
        f_color = vec4(ONE, ONE, ONE, texture(font_texture, v_in.tex_coord).r);
    }
    else {
        f_color = vec4(ONE, ONE, ONE, sample_line());
    }
}