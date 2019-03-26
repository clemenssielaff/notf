#version 320 es

precision highp float;

layout (vertices = 4) out;

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
const float PI = 3.141592653589793238462643383279502884197169399375105820975;
const float HALF_SQRT2 = 0.707106781186547524400844362104849039284835937688474036588;

// patch types
const int CONVEX  = 1;
const int CONCAVE = 2;
const int TEXT    = 3;
const int STROKE  = 4;

// cap styles
const int CAP_STYLE_BUTT   = 1;
const int CAP_STYLE_ROUND  = 2;
const int CAP_STYLE_SQUARE = 3;

// joint styles
const int JOINT_STYLE_BEVEL = 1;
const int JOINT_STYLE_ROUND = 2;
const int JOINT_STYLE_MITER = 3;

// pipeline ======================================================================================================== //

#define ID gl_InvocationID

#define START_VERTEX (gl_in[0].gl_Position.xy)
#define END_VERTEX   (gl_in[1].gl_Position.xy)

uniform int patch_type;
uniform int joint_style;
uniform int cap_style;
uniform float stroke_width;

in VertexData {
    vec2 left_ctrl;
    vec2 right_ctrl;
} vec_in[];

patch out PatchData {
    float ctrl1_length;
    float ctrl2_length;
    vec2 ctrl1_direction;
    vec2 ctrl2_direction;
    float aa_width;
    int patch_type;
} patch_out;
#define GLYPH_MIN_CORNER (patch_out.ctrl1_direction)
#define GLYPH_MAX_CORNER (patch_out.ctrl2_direction)

// general ========================================================================================================= //

/// Best fit for the following dataset:
///     Radius | Good looking tesselation
///     -------+--------------------------
///        12  |  16
///        24  |  24
///        48  |  32
///        96  |  64
///       192  |  96
///       384  | 128
///
/// see https://www.wolframalpha.com/input/?i=fit+((12,+16),+(24,+24),+(48,+32),+(96,+64),+(192,+96),+(384,+128))
float radius_to_tesselation(float radius) {
    return 8.43333 + (0.615398*radius) - (0.00079284 * radius * radius);
}

/// Cross product for 2D vectors.
float cross2(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

/// Test a bitset against given flag/s.
/// @param bitset       Integer bitset to test.
/// @param flag         Integer flag/s to test for.
/// @returns            True if all requested flags are set in the bitset.
bool test(int bitset, int flag) {
    return (bitset & flag) != 0;
}

// main ============================================================================================================ //

void main(){

    // vertex position pass-through
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;

    // we only need to calculate the patch data once
    if (ID != 0){
        return;
    }

    // the patch type bitset must always be initialized to zero
    patch_out.patch_type = 0;

    // add half a pixel diagonal to the width of the spline for anti-aliasing
    patch_out.aa_width = HALF_SQRT2;

    // text patches are a special case
    if(patch_type == TEXT) {
        patch_out.patch_type |= TYPE_TEXT;

        // this holds the position of the glyph's min and max corners
        patch_out.ctrl1_direction = vec_in[0].left_ctrl;
        patch_out.ctrl2_direction = vec_in[0].right_ctrl;

        // glyphs are mapped into a simple rectangle patch
        gl_TessLevelInner[0] = 0.;
        gl_TessLevelInner[1] = 0.;

        gl_TessLevelOuter[0] = 1.;
        gl_TessLevelOuter[1] = 1.;
        gl_TessLevelOuter[2] = 1.;
        gl_TessLevelOuter[3] = 1.;

        return;
    }

    // distance of the control points to their associated vertex
    patch_out.ctrl1_length = length(vec_in[0].right_ctrl);
    patch_out.ctrl2_length = length(vec_in[1].left_ctrl);

    // direction of the control points from their associated vertex
    patch_out.ctrl1_direction = vec_in[0].right_ctrl / patch_out.ctrl1_length;
    patch_out.ctrl2_direction = vec_in[1].left_ctrl / patch_out.ctrl2_length;

    // remove one unit of the magnitude (see plotter.cpp for details)
    patch_out.ctrl1_length -= 1.;
    patch_out.ctrl2_length -= 1.;

    /// Tesselation factor.
    /// Determines the number of tessellated segments on an edge along the direction of the spline.
    /// We use "equal spacing" for the edge tesselation spacing, meaning that each segment will have equal length.
    /// Since equal_spacing rounds tessellation levels to the next integer, this means that edges will "pop" as
    /// tessellation levels go from one integer to the next:
    ///
    ///     1: X-----------X
    ///     2: X-----X-----X
    ///     3: X---X---X---X
    ///     4: X--X--X--X--X
    ///     ...
    float tesselation = 1.;

    // convex fill
    if(patch_type == CONVEX){
        patch_out.patch_type |= TYPE_FILL | TYPE_FILL_CONVEX;
    }

    // concave fill
    else if (patch_type == CONCAVE){
        patch_out.patch_type |= TYPE_FILL | TYPE_FILL_CONCAVE;
    }

    else if(patch_type == STROKE){
        patch_out.patch_type |= TYPE_STROKE;

        // line segment stroke
        if(gl_in[0].gl_Position != gl_in[1].gl_Position){
            patch_out.patch_type |= TYPE_STROKE_SEGMENT;
            tesselation = 64.; // TODO: do something smart for adaptive line tesselation
        }

        // joint / cap
        else{
            float half_width = stroke_width / 2.;

            // start cap
            if (vec_in[0].left_ctrl == vec2(0)){
                patch_out.patch_type |= TYPE_STROKE_START_CAP;
            }

            // end cap
            else if (vec_in[1].right_ctrl == vec2(0)){
                patch_out.patch_type |= TYPE_STROKE_END_CAP;
            }

            // joint
            else {
                patch_out.patch_type |= TYPE_STROKE_JOINT;
            }

            // joint style
            if(test(patch_out.patch_type, TYPE_STROKE_JOINT)){

                // bevel joint
                if (joint_style == JOINT_STYLE_BEVEL){
                    patch_out.patch_type |= TYPE_STROKE_JOINT_BEVEL;
                }
                else {
                    float angle = acos(dot(patch_out.ctrl1_direction, -patch_out.ctrl2_direction));

                    // round joint
                    if (joint_style == JOINT_STYLE_ROUND) {
                        patch_out.patch_type |= TYPE_STROKE_JOINT_ROUND;
                        tesselation = ceil(radius_to_tesselation(HALF_SQRT2 + half_width) * (angle / PI*2.));
                    }

                    // miter joint
                    else {
                        if(half_width / sin(angle / 2.) < stroke_width * HALF_SQRT2){
                            patch_out.patch_type |= TYPE_STROKE_JOINT_MITER_CLIPPED;
                            tesselation = 3.;
                        } else {
                            patch_out.patch_type |= TYPE_STROKE_JOINT_MITER;
                            tesselation = 2.;
                        }
                    }
                }
            }

            // cap style
            else {

                // butt cap
                if(cap_style == CAP_STYLE_BUTT){
                    patch_out.patch_type |= TYPE_STROKE_CAP_BUTT;
                }

                // round cap
                else if (cap_style == CAP_STYLE_ROUND) {
                    patch_out.patch_type |= TYPE_STROKE_CAP_ROUND;
                    tesselation = ceil(radius_to_tesselation(HALF_SQRT2 + half_width) / 2.);
                }

                // square cap
                else {
                    patch_out.patch_type |= TYPE_STROKE_CAP_SQUARE;
                }
            }
        }
    }

    // apply tesselation factor
    if(test(patch_out.patch_type, TYPE_STROKE)){

        // joints only tesselate the outer side
        float left_tesselation, right_tesselation;
        if(test(patch_out.patch_type, TYPE_STROKE_JOINT)){
            bool is_left_turn = cross2(patch_out.ctrl1_direction, -patch_out.ctrl2_direction) < 0.;
            left_tesselation = is_left_turn ? 1. : tesselation;
            right_tesselation = is_left_turn ? tesselation : 1.;
        } else {
            left_tesselation = tesselation;
            right_tesselation = tesselation;
        }

        gl_TessLevelInner[0] = tesselation;
        gl_TessLevelInner[1] = 1.; // one segment along the width of the line

        gl_TessLevelOuter[0] = 1.;
        gl_TessLevelOuter[1] = left_tesselation;
        gl_TessLevelOuter[2] = 1.;
        gl_TessLevelOuter[3] = right_tesselation;
    }
    else {
        // TODO: fill calls produce quads only, but they need one side to tesselate
        //       also, would it be possible to produce a triangle, with the far side tesselated instead of producing
        //       a trowaway triangle?
        gl_TessLevelInner[0] = 0.;
        gl_TessLevelInner[1] = 0.;

        gl_TessLevelOuter[0] = 1.;
        gl_TessLevelOuter[1] = 1.;
        gl_TessLevelOuter[2] = 1.;
        gl_TessLevelOuter[3] = 1.;
    }

    // some lines don't need an additional antialiasing border
    if(test(patch_out.patch_type, TYPE_STROKE_SEGMENT)
            && mod(stroke_width, 2.) == 1.                                        // odd, integral width
            && (START_VERTEX.x == END_VERTEX.x || START_VERTEX.y == END_VERTEX.y) // horizontal or vertical
            && (fract(START_VERTEX + vec2(.5)) == vec2(0))){                      // through pixel centers
        patch_out.aa_width = 0.;
    }
}
