#version 320 es

precision highp float;

layout (quads, fractional_even_spacing) in;

// constants ======================================================================================================== //

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

/// pi, obviously
const float PI = 3.141592653589793238462643383279502884197169399375105820975;

 // patch types
const int CONVEX    = 1;
const int CONCAVE   = 2;
const int TEXT      = 3;
const int STROKE    = 4;
const int JOINT     = 41;
const int START_CAP = 42;
const int END_CAP   = 43;

// cap styles
const int CAP_STYLE_BUTT = 1;
const int CAP_STYLE_ROUND = 2;
const int CAP_STYLE_SQUARE = 3;

// joint styles
const int JOINT_STYLE_MITER = 1;
const int JOINT_STYLE_ROUND = 2;
const int JOINT_STYLE_BEVEL = 3;

// uniforms ======================================================================================================== //

#define START_VERTEX (gl_in[0].gl_Position.xy)
#define END_VERTEX   (gl_in[1].gl_Position.xy)

uniform float stroke_width;
uniform mat4 projection;
uniform int joint_style;
uniform int cap_style;
uniform vec2 vec2_aux1;
#define base_vertex vec2_aux1
#define atlas_size  vec2_aux1

// pipeline ======================================================================================================== //

patch in PatchData {
    float ctrl1_length;
    float ctrl2_length;
    vec2 ctrl1_direction;
    vec2 ctrl2_direction;
    float aa_width;
    int type;
} patch_in;
#define glyph_min_corner (patch_in.ctrl1_direction)
#define glyph_max_corner (patch_in.ctrl2_direction)

out FragmentData {
    mediump flat vec2 line_origin;
    mediump flat vec2 line_direction;
    mediump flat mat3x2 line_xform;
    mediump flat float line_half_width;
    mediump flat int patch_type;
    mediump flat int cap_style;
    mediump flat int joint_style;
    mediump vec2 texture_coord;
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

// main ============================================================================================================ //

void main()
{
    // fill fragment data
    frag_out.line_origin = START_VERTEX;
    frag_out.patch_type = patch_in.type;
    frag_out.cap_style = cap_style;
    frag_out.joint_style = joint_style;
    frag_out.texture_coord = gl_TessCoord.xy;

    vec2 line_run = END_VERTEX - START_VERTEX;
    float line_length = length(line_run);
    float half_width = stroke_width / 2.;
    float style_offset = cap_style == CAP_STYLE_SQUARE ? half_width : 1.;

    if(patch_in.type == STROKE){
        frag_out.line_direction = line_length == 0. ? patch_in.ctrl1_direction
                                                    : line_run / line_length;
    } else if (patch_in.type == JOINT){
        frag_out.line_direction = normalize(patch_in.ctrl1_direction - patch_in.ctrl2_direction);
    } else if (patch_in.type == START_CAP){
        frag_out.line_direction = patch_in.ctrl1_direction;
    } else if (patch_in.type == END_CAP){
        frag_out.line_direction = -patch_in.ctrl2_direction;
    }

    if(patch_in.type == JOINT && joint_style != JOINT_STYLE_ROUND){
        frag_out.line_half_width = length(closest_point_on_line(
                vec2(0), orthogonal(patch_in.ctrl2_direction) * half_width, frag_out.line_direction));
    } else {
        frag_out.line_half_width = half_width;
    }

    frag_out.line_xform = mat3x2(
        vec2(frag_out.line_direction.x, -frag_out.line_direction.y),
        vec2(frag_out.line_direction.y,  frag_out.line_direction.x),
        vec2(dot(frag_out.line_direction, -START_VERTEX),
             cross2(frag_out.line_direction, -START_VERTEX))
    );


    vec2 vertex_pos;
    if(patch_in.type == CONVEX){
        // frag_out.texture_coord.y = patch_in.aa_width == 0.0 ? 1.0 : 1. - step(0.9, gl_TessCoord.y);

        // This always creates a triangle with zero area :/ but I hope that the GPU is quick to discard such polygons.
        // The alternative would be that I always pass 3 vertices to a patch, which would mean that I always pass an unused
        // vertex for each line segment, which might be slower
        vec2 delta = mix(END_VERTEX, START_VERTEX, gl_TessCoord.x) - base_vertex;
        // vertex_pos = fma(vec2(step(.5, gl_TessCoord.y) * (length(delta) - (sign(frag_out.texture_coord.y - .5) * patch_in.aa_width))),
        //                      normalize(delta), base_vertex);
        vertex_pos = fma(vec2(step(.5, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (patch_in.type == CONCAVE) {
        // see comment in the CONVEX branch
        vec2 delta = mix(END_VERTEX, START_VERTEX, gl_TessCoord.x) - base_vertex;
        vertex_pos = fma(vec2(step(.5, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (patch_in.type == TEXT) {
        vec2 uv_max = START_VERTEX + (glyph_max_corner - glyph_min_corner);
        frag_out.texture_coord = mix(START_VERTEX, uv_max, vec2(gl_TessCoord.x, 1. - gl_TessCoord.y)) / atlas_size;
        vertex_pos = mix(glyph_min_corner, glyph_max_corner, gl_TessCoord.xy);
    }

    else { // patch_in.type is some form of line
        float normal_offset = (patch_in.aa_width + half_width) * sign(gl_TessCoord.y - .5);

        if(patch_in.type == STROKE){
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

        else if(patch_in.type == JOINT){
            if(joint_style == JOINT_STYLE_ROUND){
                float start_angle = atan(-patch_in.ctrl1_direction.x, patch_in.ctrl1_direction.y);
                float end_angle = atan(patch_in.ctrl2_direction.x, -patch_in.ctrl2_direction.y);
                float angle;
                if(abs(start_angle - end_angle) <= PI){
                    angle = mix(start_angle, end_angle, gl_TessCoord.x);
                } else {
                    angle = mix(end_angle - PI, start_angle + PI, gl_TessCoord.x);
                }
                vec2 spoke_direction = vec2(cos(angle), sin(angle));
                vertex_pos = START_VERTEX + (spoke_direction * normal_offset);
            }
            else {
                vec2 normal;
                if(gl_TessCoord.x == 0.){
                    normal = orthogonal(-patch_in.ctrl1_direction);
                } else {
                    normal = orthogonal(patch_in.ctrl2_direction);
                }
                vertex_pos = START_VERTEX + (normal * normal_offset);
            }
        }

        else if(patch_in.type == START_CAP){
            if(cap_style == CAP_STYLE_ROUND){
                float angle = fma(PI, gl_TessCoord.x, atan(-frag_out.line_direction.x, frag_out.line_direction.y));
                vec2 spoke_direction = vec2(cos(angle), sin(angle));
                vertex_pos = START_VERTEX + (spoke_direction * normal_offset);
            }
            else {
                float run_offset = (gl_TessCoord.x  == 0.) ? -style_offset : 0.;
                vertex_pos = START_VERTEX + (run_offset * frag_out.line_direction)                    // along
                                          + (normal_offset * orthogonal(frag_out.line_direction));    // normal
            }
        }

        else if(patch_in.type == END_CAP){
            if(cap_style == CAP_STYLE_ROUND){
                float angle = fma(PI, gl_TessCoord.x, atan(frag_out.line_direction.x, -frag_out.line_direction.y));
                vec2 spoke_direction = vec2(cos(angle), sin(angle));
                vertex_pos = START_VERTEX + (spoke_direction * normal_offset);
            }
            else {
                float run_offset = (gl_TessCoord.x == 0.) ? 0. : style_offset;
                vertex_pos = START_VERTEX + (run_offset * frag_out.line_direction)                    // along
                                          + (normal_offset * orthogonal(frag_out.line_direction));    // normal
            }
        }
    }

    // clip-space position of the vertex
    gl_Position = projection * vec4(vertex_pos, -1, 1);

}

