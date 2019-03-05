#version 320 es

precision mediump float;

in VertexData {
    mat3x2 screen_to_line_xform;
    vec2 line_size;
    vec2 texture_coord;
    flat int patch_type;
} v_in;

uniform sampler2D font_texture;

layout(std140) uniform PaintBlock {
    vec4 paint_rotation;    //  0 (size = 4)
    vec2 paint_translation; //  4 (size = 2)
    vec2 paint_size;        //  6 (size = 2)
    vec4 clip_rotation;     //  8 (size = 4)
    vec2 clip_translation;  // 12 (size = 2)
    vec2 clip_size;         // 14 (size = 2)
    vec4 inner_color;       // 16 (size = 4)
    vec4 outer_color;       // 20 (size = 4)
    int type;               // 24 (size = 1)
    float stroke_width;     // 25 (size = 1)
    float radius;           // 26 (size = 1)
    float feather;          // 27 (size = 1)
};

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
    const vec4 pattern_x = vec4(1., 7., 11., 15.) / 32.;
    const vec4 pattern_y = vec4(5., 13., 3., 9.) / 32.;
    const vec4 weights = vec4(1);               // uniform weights across all sample points
    const vec2 signs[4] = vec2[4](vec2(+1, +1), // top-right quadrant
                                  vec2(+1, -1), // bottom-right quadrant
                                  vec2(-1, -1), // bottom-left quadrant
                                  vec2(-1, +1));// top-left quadrant

    float result = 0.;
    for(int quadrant = 0; quadrant < 4; ++quadrant){
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = (v_in.screen_to_line_xform * vec3(fma(vec2(pattern_x[i], pattern_y[i]), signs[quadrant], gl_FragCoord.xy), 1)).xy;
            samples_x[i] = sample_pos.x;
            samples_y[i] = sample_pos.y;
        }
        samples_x = step(0., samples_x) - step(v_in.line_size.x, samples_x);
        samples_y = step(0., samples_y) - step(v_in.line_size.y, samples_y);
        result += dot(step(2., samples_x + samples_y), weights);
    }
    return result / 16.;
}

/// Signed distance to a rounded rectangle centered at the origin, used for all gradients.
/// * Box gradients use the rounded rectangle as it is.
/// * Circle gradients use a rounded rectangle where size = (radius, radius).
/// * Linear gradients are set up to use only one edge of a very large rectangle.
/// *Thanks to Mikko Mononen for the function and helpful explanation!*
/// See https://github.com/memononen/nanovg/issues/369
/// @param point    The distance to this point is calculated.
/// @param size     Size of the rectangle.
/// @param radius   Radius of the rounded corners of the rectangle.
float get_rounded_rect_distance(vec2 point, vec2 size, float radius) {
    vec2 inner_size = size - vec2(radius, radius);
    vec2 delta = abs(point) - inner_size;
    return min(max(delta.x, delta.y), 0.) + length(max(delta, 0.)) - radius;
}

/// Scissor factor from a rotated rectangle defined in the paint.
float get_clipping_factor() {
    mat3x2 clip_xform = mat3x2(clip_rotation, clip_translation);
    vec2 clip = vec2(0.5, 0.5) - (abs((clip_xform * vec3(gl_FragCoord.xy, 1.)).xy) - clip_size);
    return clamp(clip.x, 0., 1.) * clamp(clip.y, 0., 1.);
}

void main()
{
    float clipping_factor = get_clipping_factor();
    if(clipping_factor == 0.){
        discard;
    }

    if(v_in.patch_type == TEXT){
        f_color = vec4(1, 1, 1, texture(font_texture, v_in.texture_coord).r);
    }
    else if(v_in.patch_type == STROKE){
        float aa_factor = sample_line();
        if(aa_factor == 0.){
            discard;
        }
        f_color = vec4(1, 1, 1, aa_factor);
    }
    else {
        f_color = vec4(1, 1, 1, 1);
    }
}
