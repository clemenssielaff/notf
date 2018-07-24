#version 320 es

precision mediump float;

in VertexData {
    mat2x3 line_xform;
    vec2 line_size;
    vec2 position;
    vec2 tex_coord;
    flat int patch_type;
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

// float sample_line()
// {
//     const float STEP = 1.0 / 8.0;
//     vec2 pixel = gl_FragCoord.xy;
//     float result = 0.;
//     vec3 pos = vec3(1.);
//     for(float x = 0.; x < 4.; ++x){
//         pos.x = pixel.x + ((2. * x) + 1.) * STEP;
//         for(float y = 0.; y < 4.; ++y){
//             pos.y = pixel.y + ((2. * y) + 1.) * STEP;
//             vec2 sample_pos = pos * v_in.line_xform;
//             if(sample_pos.x >= 0. && sample_pos.x <= v_in.line_size.x &&
//                sample_pos.y >= 0. && sample_pos.y <= v_in.line_size.y){
//                 result += 1.0 / 16.0;
//             }
//         }
//     }
//     return result;
// }


float sample_line()
{
    const vec4 pattern_x = vec4(1.0, 7.0, 11.0, 15.0) / 32.0;
    const vec4 pattern_y = vec4(3.0, 5.0, 9.0, 13.0) / 32.0;
    
    float result = 0.0;
    
    {// top-right quadrant
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = vec3(gl_FragCoord.xy + vec2(pattern_x[i], pattern_y[i]), 1.) * v_in.line_xform;
            samples_x[i] = sample_pos.x;
            samples_y[i] = sample_pos.y;
        }
        samples_x = step(0., samples_x) - step(v_in.line_size.x, samples_x);
        samples_y = step(0., samples_y) - step(v_in.line_size.y, samples_y);
        result += dot(step(1.9, samples_x + samples_y), vec4(1.0)) / 16.0;
    }
    {// bottom-right quadrant
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = vec3(gl_FragCoord.xy + vec2(pattern_x[i], -pattern_y[i]), 1.) * v_in.line_xform;
            samples_x[i] = sample_pos.x;
            samples_y[i] = sample_pos.y;
        }
        samples_x = step(0., samples_x) - step(v_in.line_size.x, samples_x);
        samples_y = step(0., samples_y) - step(v_in.line_size.y, samples_y);
        result += dot(step(1.9, samples_x + samples_y), vec4(1.0)) / 16.0;
    }
    {// bottom-left quadrant
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = vec3(gl_FragCoord.xy - vec2(pattern_x[i], pattern_y[i]), 1.) * v_in.line_xform;
            samples_x[i] = sample_pos.x;
            samples_y[i] = sample_pos.y;
        }
        samples_x = step(0., samples_x) - step(v_in.line_size.x, samples_x);
        samples_y = step(0., samples_y) - step(v_in.line_size.y, samples_y);
        result += dot(step(1.9, samples_x + samples_y), vec4(1.0)) / 16.0;
    }
    {// top-left quadrant
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = vec3(gl_FragCoord.xy + vec2(-pattern_x[i], pattern_y[i]), 1.) * v_in.line_xform;
            samples_x[i] = sample_pos.x;
            samples_y[i] = sample_pos.y;
        }
        samples_x = step(0., samples_x) - step(v_in.line_size.x, samples_x);
        samples_y = step(0., samples_y) - step(v_in.line_size.y, samples_y);
        result += dot(step(1.9, samples_x + samples_y), vec4(1.0)) / 16.0;
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