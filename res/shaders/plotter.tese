#version 320 es

// defines ========================================================================================================= //

// Some but not all systems support a separate floating point precision for different stages.
// In that case, it might save some space to use a lower precision for the fragment shader.
// By default we use high-precision floating point operations in all stages because that seems to work more often.
#ifndef NOTF_FRAGMENT_FLOAT_PRECISION
#define NOTF_FRAGMENT_FLOAT_PRECISION
#endif

// stage specific ================================================================================================== //

precision highp float;

layout (quads, equal_spacing) in;

// constants ======================================================================================================= //

/// cubic bezier matrix
const mat4 BEZIER = mat4(
     1,  0,  0,  0,
    -3,  3,  0,  0,
     3, -6,  3,  0,
    -1,  3, -3,  1);

/// cubic bezier derivation matrix
const mat3 DERIV = mat3(
     3,  0,  0,
    -6,  6,  0,
     3, -6,  3);

// constant symbols
const float PI = 3.141592653589793238462643383279502884197169399375105820975;
const float HALF_SQRT2 = 0.707106781186547524400844362104849039284835937688474036588;

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

// pipeline ======================================================================================================== //

#define START_VERTEX (gl_in[0].gl_Position.xy)
#define END_VERTEX   (gl_in[1].gl_Position.xy)

uniform float stroke_width;
uniform mat4 projection;
uniform vec2 vec2_aux1;
#define base_vertex vec2_aux1
#define atlas_size  vec2_aux1

patch in PatchData {
    float ctrl1_length;
    float ctrl2_length;
    vec2 ctrl1_direction;
    vec2 ctrl2_direction;
    float aa_width;
    int patch_type;
} patch_in;
#define GLYPH_MIN_CORNER (patch_in.ctrl1_direction)
#define GLYPH_MAX_CORNER (patch_in.ctrl2_direction)

out FragmentData {
    mediump flat int patch_type;
    NOTF_FRAGMENT_FLOAT_PRECISION flat float line_half_width;
    NOTF_FRAGMENT_FLOAT_PRECISION flat vec2 line_origin;
    NOTF_FRAGMENT_FLOAT_PRECISION flat mat3x2 line_xform;
    NOTF_FRAGMENT_FLOAT_PRECISION vec2 texture_coord;
} frag_out;

// general ========================================================================================================= //

/// Cross product for 2D vectors.
float cross2(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

/// A 2D vector 90deg rotated counter-clockwise.
vec2 orthogonal(vec2 vec) {
    return vec.yx * vec2(-1, 1);
}

/// Calculates the point closest to `point` on the infinite line defined by `anchor` and `direction`.
/// @param point        Point with minimal distance to the result.
/// @param anchor       Any point on the line.
/// @param direction    Direction of the line, must be normalized.
vec2 closest_point_on_line(vec2 point, vec2 anchor, vec2 direction) {
    return anchor + (direction * dot((point-anchor), direction));
}

/// Test a bitset against given flag/s.
/// @param bitset       Integer bitset to test.
/// @param flag         Integer flag/s to test for.
/// @returns            True if all requested flags are set in the bitset.
bool test(int bitset, int flag) {
    return (bitset & flag) != 0;
}

/// Spherical blend between start and end vector.
/// From https://www.shadertoy.com/view/4sV3zt
/// @param start    Start vector, must be normalized.
/// @param end      End vector, must be normalized.
/// @param blend    Blend factor in the range [0, 1].
/// @returns        The interpolated vector, also normalized.
vec2 slerp(vec2 start, vec2 end, float blend) {
     float cos_angle = clamp(dot(start, end), -1., 1.);
     float weighted_angle = acos(cos_angle) * blend;
     return (start * cos(weighted_angle)) + (normalize(end - start*cos_angle) * sin(weighted_angle));
}

// main ============================================================================================================ //

void main()
{
    // fill fragment data
    frag_out.line_origin = START_VERTEX;
    frag_out.patch_type = patch_in.patch_type;
    frag_out.texture_coord = gl_TessCoord.xy;

    // TODO: I heavily suspect that there are operations in the following code that could be consolidated, especially
    //       considering that reduced branching might be faster than fewer calculations
    vec2 line_run = END_VERTEX - START_VERTEX;
    float line_length = length(line_run);
    float half_width = stroke_width / 2.;
    bool is_left_turn = cross2(patch_in.ctrl1_direction, -patch_in.ctrl2_direction) < 0.;
    float style_offset = test(patch_in.patch_type, TYPE_STROKE_CAP_SQUARE) ? half_width : 1.;

    vec2 line_direction;
    if(test(patch_in.patch_type, TYPE_STROKE_SEGMENT)){
        line_direction = line_length == 0. ? patch_in.ctrl1_direction : line_run / line_length;
    } else if (test(patch_in.patch_type, TYPE_STROKE_JOINT)){
        line_direction = normalize(patch_in.ctrl1_direction - patch_in.ctrl2_direction);
    } else if (test(patch_in.patch_type, TYPE_STROKE_START_CAP)){
        line_direction = patch_in.ctrl1_direction;
    } else if (test(patch_in.patch_type, TYPE_STROKE_END_CAP)){
        line_direction = -patch_in.ctrl2_direction;
    }

    if(test(patch_in.patch_type, TYPE_STROKE_JOINT) && !test(patch_in.patch_type, TYPE_STROKE_JOINT_ROUND)){
        frag_out.line_half_width = length(closest_point_on_line(
                vec2(0), orthogonal(patch_in.ctrl2_direction) * half_width, line_direction));
    } else {
        frag_out.line_half_width = half_width;
    }

    frag_out.line_xform = mat3x2(
        vec2(line_direction.x, -line_direction.y),
        vec2(line_direction.y,  line_direction.x),
        vec2(dot(line_direction, -START_VERTEX),
             cross2(line_direction, -START_VERTEX))
    );

    vec2 vertex_pos;
    if(test(patch_in.patch_type, TYPE_FILL_CONVEX)){
        // frag_out.texture_coord.y = patch_in.aa_width == 0.0 ? 1.0 : 1. - step(0.9, gl_TessCoord.y);

        // This always creates a triangle with zero area :/ but I hope that the GPU is quick to discard such polygons.
        // The alternative would be that I always pass 3 vertices to a patch, which would mean that I always pass an unused
        // vertex for each line segment, which might be slower
        vec2 delta = mix(END_VERTEX, START_VERTEX, gl_TessCoord.x) - base_vertex;
        // vertex_pos = fma(vec2(step(.5, gl_TessCoord.y) * (length(delta) - (sign(frag_out.texture_coord.y - .5) * patch_in.aa_width))),
        //                      normalize(delta), base_vertex);
        vertex_pos = fma(vec2(step(.5, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (test(patch_in.patch_type, TYPE_FILL_CONCAVE)) {
        // see comment in the CONVEX branch
        vec2 delta = mix(END_VERTEX, START_VERTEX, gl_TessCoord.x) - base_vertex;
        vertex_pos = fma(vec2(step(.5, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (test(patch_in.patch_type, TYPE_TEXT)) {
        vec2 uv_max = START_VERTEX + (GLYPH_MAX_CORNER - GLYPH_MIN_CORNER);
        frag_out.texture_coord = mix(START_VERTEX, uv_max, vec2(gl_TessCoord.x, 1. - gl_TessCoord.y)) / atlas_size;
        vertex_pos = mix(GLYPH_MIN_CORNER, GLYPH_MAX_CORNER, gl_TessCoord.xy);
    }

    else { // patch_in.patch_type is some form of line
        float normal_offset = (half_width + patch_in.aa_width) * sign(gl_TessCoord.y - .5);

        if(test(patch_in.patch_type, TYPE_STROKE_SEGMENT)){
            // bezier control points
            vec2 ctrl1 = START_VERTEX + (patch_in.ctrl1_direction * patch_in.ctrl1_length);
            vec2 ctrl2 = END_VERTEX + (patch_in.ctrl2_direction * patch_in.ctrl2_length);

            // screen space position of the vertex
            vec2 tmp = vec2(1, gl_TessCoord.x);
            mat4x2 bezier = mat4x2(START_VERTEX, ctrl1, ctrl2, END_VERTEX) * BEZIER;
            mat3x2 derivation = mat3x2(ctrl1 - START_VERTEX, ctrl2 - ctrl1, END_VERTEX - ctrl2) * DERIV;
            vec2 normal = orthogonal(normalize(derivation * (tmp.xyy * tmp.xxy)));
            vertex_pos = (bezier * (tmp.xyyy * tmp.xxyy * tmp.xxxy)).xy // position along line
                        + (normal * normal_offset);                     // position along normal
        }

        else if(test(patch_in.patch_type, TYPE_STROKE_JOINT)){

            // the inner side of the joint is always at the center
            if(gl_TessCoord.y == (is_left_turn ? 0. : 1.)){
                vertex_pos = START_VERTEX;
            }

            // outer (tesselated) side
            else {
                vec2 joint_start_direction = orthogonal(-patch_in.ctrl1_direction);
                vec2 joint_end_direction = orthogonal(patch_in.ctrl2_direction);

                // round joint
                if(test(patch_in.patch_type, TYPE_STROKE_JOINT_ROUND)){
                    vec2 direction = slerp(joint_start_direction, joint_end_direction, gl_TessCoord.x);
                    vertex_pos = START_VERTEX + (direction * normal_offset);
                }

                // miter joint
                else if(test(patch_in.patch_type, TYPE_STROKE_JOINT_MITER | TYPE_STROKE_JOINT_MITER_CLIPPED)){
                    vec2 start = START_VERTEX + (joint_start_direction * normal_offset);
                    vec2 end = START_VERTEX + (joint_end_direction * normal_offset);
                    float angle = acos(dot(patch_in.ctrl1_direction, patch_in.ctrl2_direction));
                    vec2 direction = normalize(patch_in.ctrl1_direction + patch_in.ctrl2_direction);
                    float distance = half_width / sin(angle / 2.);
                    vec2 miter_point = START_VERTEX - (direction * distance);
                    if(gl_TessCoord.x == 0.){
                        vertex_pos = start;
                    } else if(gl_TessCoord.x == 1.){
                        vertex_pos = end;
                    } else if(test(patch_in.patch_type, TYPE_STROKE_JOINT_MITER)){
                        vertex_pos = miter_point;
                    } else if (abs(gl_TessCoord.x - 1./3.) < 0.1){
                        vertex_pos = start + normalize(miter_point - start) * half_width;
                    } else {
                        vertex_pos = end + normalize(miter_point - end) * half_width;
                    }
                }

                // bevel joint
                else {
                    vec2 direction = mix(joint_start_direction, joint_end_direction, gl_TessCoord.x);
                    vertex_pos = START_VERTEX + (direction *  normal_offset);
                }
            }
        }

        else if(test(patch_in.patch_type, TYPE_STROKE_START_CAP)){
            if(test(patch_in.patch_type, TYPE_STROKE_CAP_ROUND)){
                float angle = fma(PI, gl_TessCoord.x, atan(-line_direction.x, line_direction.y));
                vec2 spoke_direction = vec2(cos(angle), sin(angle));
                vertex_pos = START_VERTEX + (spoke_direction * normal_offset);
            }
            else {
                float run_offset = (gl_TessCoord.x  == 0.) ? -style_offset : 0.;
                vertex_pos = START_VERTEX + (run_offset * line_direction)                    // along
                                          + (normal_offset * orthogonal(line_direction));    // normal
            }
        }

        else if(test(patch_in.patch_type, TYPE_STROKE_END_CAP)){
            if(test(patch_in.patch_type, TYPE_STROKE_CAP_ROUND)){
                float angle = fma(PI, gl_TessCoord.x, atan(line_direction.x, -line_direction.y));
                vec2 spoke_direction = vec2(cos(angle), sin(angle));
                vertex_pos = START_VERTEX + (spoke_direction * normal_offset);
            }
            else {
                float run_offset = (gl_TessCoord.x == 0.) ? 0. : style_offset;
                vertex_pos = START_VERTEX + (run_offset * line_direction)                    // along
                                          + (normal_offset * orthogonal(line_direction));    // normal
            }
        }
    }

    // clip-space position of the vertex
    gl_Position = projection * vec4(vertex_pos, -1, 1);

}

