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

/// Cheap but precise super-sampling of the fragment coverage by the rendered line.
float sample_line()
{
    // for a visual reference of the sample pattern see {notf_root}/dev/diagrams/aa_pattern.svg
    const vec4 pattern_x = vec4(1.0, 7.0, 11.0, 15.0) / 32.0;
    const vec4 pattern_y = vec4(5.0, 13.0, 3.0, 9.0) / 32.0;
    const vec4 weights = vec4(1);               // uniform weights across all sample points
    const vec2 signs[4] = vec2[4](vec2(+1, +1), // top-right quadrant
                                  vec2(+1, -1), // bottom-right quadrant
                                  vec2(-1, -1), // bottom-left quadrant
                                  vec2(-1, +1));// top-left quadrant
    
    float result = 0.;
    for(int quadrant = 0; quadrant < 4; ++quadrant){
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = vec3(
                fma(vec2(pattern_x[i], pattern_y[i]), signs[quadrant], gl_FragCoord.xy), 1) * v_in.line_xform;
            samples_x[i] = sample_pos.x;
            samples_y[i] = sample_pos.y;
        }
        samples_x = step(0., samples_x) - step(v_in.line_size.x, samples_x);
        samples_y = step(0., samples_y) - step(v_in.line_size.y, samples_y);
        result += dot(step(2., samples_x + samples_y), weights);
    }
    return result / 16.;
}

void main() 
{
    if(v_in.patch_type == TEXT){
        f_color = vec4(1, 1, 1, texture(font_texture, v_in.tex_coord).r);
    }
    else if(v_in.patch_type == STROKE){
        f_color = vec4(1, 1, 1, sample_line());
    }
    else {
        f_color = vec4(1, 1, 1, 1);
    }
}
