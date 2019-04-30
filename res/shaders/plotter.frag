#version 320 es

// defines ========================================================================================================= //

// Some but not all systems support a separate floating point precision for different stages.
// In that case, it might save some space to use a lower precision for the fragment shader.
// By default we use high-precision floating point operations in all stages because that seems to work more often.
#ifndef NOTF_FRAGMENT_FLOAT_PRECISION
#   ifdef GL_FRAGMENT_PRECISION_HIGH
#       define NOTF_FRAGMENT_FLOAT_PRECISION highp
#   else
#       define NOTF_FRAGMENT_FLOAT_PRECISION mediump
#   endif
#endif

// stage specific ================================================================================================== //

precision NOTF_FRAGMENT_FLOAT_PRECISION float;

// constants ======================================================================================================== //

// type flags
const int TYPE_FILL   = 1 << 0;
const int TYPE_STROKE = 1 << 1;
const int TYPE_TEXT   = 1 << 2;

const int TYPE_FILL_CONVEX  = 1 << 3;
const int TYPE_FILL_CONCAVE = 1 << 4;

const int TYPE_STROKE_SEGMENT   = 1 << 5;
const int TYPE_STROKE_JOINT     = 1 << 6;
const int TYPE_STROKE_START_CAP = 1 << 7;
const int TYPE_STROKE_END_CAP   = 1 << 8;

const int TYPE_STROKE_JOINT_BEVEL         = 1 <<  9;
const int TYPE_STROKE_JOINT_ROUND         = 1 << 10;
const int TYPE_STROKE_JOINT_MITER         = 1 << 11;
const int TYPE_STROKE_JOINT_MITER_CLIPPED = 1 << 12;

const int TYPE_STROKE_CAP_BUTT   = 1 << 13;
const int TYPE_STROKE_CAP_ROUND  = 1 << 14;
const int TYPE_STROKE_CAP_SQUARE = 1 << 15;

// constant symbols
const float IGNORED = 0.; // for readability

// uniforms ======================================================================================================== //

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
    float stroke_width_;    // 25 (size = 1) // TODO: there is already a `stroke_width` uniform
    float radius;           // 26 (size = 1)
    float feather;          // 27 (size = 1)
};

// pipeline ======================================================================================================== //

in FragmentData {
    /// type of patch creating this fragment
    flat int patch_type;

    /// Distance from the center of the line to its side.
    flat float line_half_width;

    /// start point of the line in screen coordinates
    flat vec2 line_origin;

    /// transformation of the origin from screen- to line-space
    flat mat3x2 line_xform;

    /// texture coordinate of this fragment (interpolated)
    vec2 texture_coord;
} frag_in;

layout(location=0) out vec4 final_color;

// general ========================================================================================================= //

/// Test a bitset against given flag/s.
/// @param bitset       Integer bitset to test.
/// @param flag         Integer flag/s to test for.
/// @returns            True if all requested flags are set in the bitset.
bool test(int bitset, int flag) {
    return (bitset & flag) != 0;
}

// antialiasing ==================================================================================================== //

/// Sample pattern.
/// for a visual reference of the sample pattern see {notf_root}/dev/diagrams/aa_pattern.svg
const vec2 SAMPLE_PATTERN[4] = vec2[](
    vec2( 1,  5)  / 32.,
    vec2( 7, 13)  / 32.,
    vec2(11,  3)  / 32.,
    vec2(15,  9)  / 32.
);

/// We apply uniform weights across all sample points.
const vec4 SAMPLE_WEIGHTS = vec4(1);

/// The samples are rotated around the origin and applied once in each quadrant.
const vec2 SAMPLE_QUADRANTS[4] = vec2[4](vec2(+1, +1),  // top-right quadrant
                                         vec2(+1, -1),  // bottom-right quadrant
                                         vec2(-1, -1),  // bottom-left quadrant
                                         vec2(-1, +1)); // top-left quadrant

/// Cheap but precise super-sampling of the fragment coverage by the rendered line.
/// @param coord        Fragment position in screen coordinates.
/// @param xform        Transformation from screen to line-space. In line space, the center start is at the origin
///                     and the end of the line is at position (length, 0).
/// @param half_length  Half the length of the line segment. Ignored if zero.
/// @param half_width   Distance from the center of the line to its side. Ignored if zero.
float sample_line(vec2 coord, mat3x2 xform, float half_length, float half_width)
{
    // transform the coordinate from screen-space into line-space
    coord = (xform * vec3(coord, 1)).xy;

    float result = 0.;
    for(int quadrant = 0; quadrant < 4; ++quadrant){
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_coord = fma(SAMPLE_PATTERN[i], SAMPLE_QUADRANTS[quadrant], coord);
            samples_x[i] = sample_coord.x;
            samples_y[i] = sample_coord.y;
        }
        samples_x = (half_length == 0.) ? vec4(1) : (step(-half_length, samples_x) - step(half_length, samples_x));
        samples_y = (half_width == 0.) ? vec4(1) : (step(-half_width, samples_y) - step(half_width, samples_y));
        result += dot(step(2., samples_x + samples_y), SAMPLE_WEIGHTS);
    }
    return result / 16.;
}

float sample_circle(vec2 coord, vec2 center, float radius_sq)
{
    float result = 0.;
    for(int quadrant = 0; quadrant < 4; ++quadrant){
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = coord + (SAMPLE_PATTERN[i] * SAMPLE_QUADRANTS[quadrant]);
            vec2 delta = sample_pos - center;
            result += (dot(delta, delta) <= radius_sq) ? 1. : 0.;
        }
    }
    return result / 16.;
}

// ================================================================================================================= //

/// Signed distance to a rounded rectangle centered at the origin, used for all gradients.
/// * Box gradients use the rounded rectangle as it is.
/// * Circle gradients use a rounded rectangle where size = (radius, radius).
/// * Linear gradients are set up to use only one edge of a very large rectangle.
/// *Thanks to Mikko Mononen for the function and helpful explanation!*
/// See https://github.com/memononen/nanovg/issues/369
/// @param point    The distance to this point is calculated.
/// @param size     Size of the rectangle.
/// @param radius   Radius of the rounded corners of the rectangle.
float get_rounded_rect_distance(vec2 point, vec2 size, float radius)
{
    vec2 inner_size = size - vec2(radius, radius);
    vec2 delta = abs(point) - inner_size;
    return min(max(delta.x, delta.y), 0.) + length(max(delta, 0.)) - radius;
}

/// Scissor factor from a rotated rectangle defined in the paint.
float get_clipping_factor()
{
    mat3x2 clip_xform = mat3x2(clip_rotation, clip_translation);
    vec2 clip = vec2(0.5, 0.5) - (abs((clip_xform * vec3(gl_FragCoord.xy, 1.)).xy) - clip_size);
    return clamp(clip.x, 0., 1.) * clamp(clip.y, 0., 1.);
}

// main ============================================================================================================ //

void main()
{
    if(get_clipping_factor() == 0.){
        discard;
    }

    vec3 color = vec3(1);
    float alpha = 0.;
    if(test(frag_in.patch_type, TYPE_TEXT)){
        color = vec3(1);
        alpha = texture(font_texture, frag_in.texture_coord).r;
    }
    else if(test(frag_in.patch_type, TYPE_STROKE_SEGMENT)){
        color = vec3(0, 1, 0);
        alpha = sample_line(gl_FragCoord.xy, frag_in.line_xform, IGNORED, frag_in.line_half_width);
    }
    else if(test(frag_in.patch_type, TYPE_STROKE_JOINT)){
        color = vec3(1, .5, 0);
        if(test(frag_in.patch_type, TYPE_STROKE_JOINT_ROUND)){
            alpha = sample_circle(gl_FragCoord.xy, frag_in.line_origin, frag_in.line_half_width * frag_in.line_half_width);
        } else if(test(frag_in.patch_type, TYPE_STROKE_JOINT_BEVEL)) {
            alpha = sample_line(gl_FragCoord.xy, frag_in.line_xform, IGNORED, frag_in.line_half_width);
        } else {
            alpha = 1.; // TODO: miter joints don't have antialiasing yet
        }
    }
    else if(test(frag_in.patch_type, TYPE_STROKE_START_CAP | TYPE_STROKE_END_CAP)){
        color = vec3(0, .5, 1);
        if(test(frag_in.patch_type, TYPE_STROKE_CAP_ROUND)){
            alpha = sample_circle(gl_FragCoord.xy, frag_in.line_origin, frag_in.line_half_width * frag_in.line_half_width);
        } else {
            float style_offset = test(frag_in.patch_type, TYPE_STROKE_CAP_SQUARE) ? frag_in.line_half_width : 1.;
            alpha = sample_line(gl_FragCoord.xy, frag_in.line_xform, style_offset, frag_in.line_half_width);
        }
    }
    if(alpha == 0.){
        discard;
    }

    // color = vec3(1);
    alpha *= .5;
    final_color = vec4(color, alpha);
}
