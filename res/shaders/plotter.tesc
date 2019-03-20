#version 320 es

precision highp float;

layout (vertices = 4) out;

#define ID gl_InvocationID

in VertexData {
    vec2 left_ctrl;
    vec2 right_ctrl;
} v_in[];

uniform int patch_type;
uniform float stroke_width;
uniform int joint_style;
uniform int cap_style;

patch out PatchData {
    float ctrl1_length;
    float ctrl2_length;
    vec2 ctrl1_direction;
    vec2 ctrl2_direction;
    float aa_width;
    int type;
} patch_out;

// constant symbols
const float ZERO = 0.0f;
const float ONE = 1.0f;
const float TWO = 2.0f;
const float THREE = 3.0f;
const float PI = 3.141592653589793238462643383279502884197169399375105820975;
const float HALF_SQRT2 = 0.707106781186547524400844362104849039284835937688474036588;

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

#define START_VERTEX (gl_in[0].gl_Position.xy)
#define END_VERTEX   (gl_in[1].gl_Position.xy)

// settings
const float tessel_x_factor = 0.0001;
const float tessel_x_max = 64.0;

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
    return ceil(8.43333 + (0.615398*radius) - (0.00079284 * radius * radius));
}

void main(){

    // vertex position pass-through
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;

    // we only need to calculate the patch data once
    if (ID != 0){
        return;
    }

    patch_out.type = patch_type;

    if(patch_type == TEXT) {
        gl_TessLevelInner[0] = ZERO;
        gl_TessLevelInner[1] = ZERO;

        gl_TessLevelOuter[0] = ONE;
        gl_TessLevelOuter[1] = ONE;
        gl_TessLevelOuter[2] = ONE;
        gl_TessLevelOuter[3] = ONE;

        // this holds the position of the glyph's min and max corners
        patch_out.ctrl1_direction = v_in[0].left_ctrl;
        patch_out.ctrl2_direction = v_in[0].right_ctrl;
    }

    // branch for everything but text
    else {
        // direction of the control points from their nearest vertex
        patch_out.ctrl1_direction = normalize(v_in[0].right_ctrl);
        patch_out.ctrl2_direction = normalize(v_in[1].left_ctrl);

        // ctrl point delta magnitude
        patch_out.ctrl1_length = max(ZERO, length(v_in[0].right_ctrl) - ONE);
        patch_out.ctrl2_length = max(ZERO, length(v_in[1].left_ctrl) - ONE);

        // bezier spline points
        vec2 ctrl1 = START_VERTEX + (patch_out.ctrl1_direction * patch_out.ctrl1_length);
        vec2 ctrl2 = END_VERTEX + (patch_out.ctrl2_direction * patch_out.ctrl2_length);

        if(patch_type == CONVEX || patch_type == CONCAVE){
            gl_TessLevelInner[0] = ZERO;
            gl_TessLevelInner[1] = ZERO;

            gl_TessLevelOuter[0] = ONE;
            gl_TessLevelOuter[1] = ONE;
            gl_TessLevelOuter[2] = ONE;
            gl_TessLevelOuter[3] = ONE;
        }

        else if(patch_type == STROKE){
            // tesselation defaults
            float tessel_x = ONE;           // along spline
            const float tessel_y = ONE;     // along normal

            // joint / caps
            if(gl_in[0].gl_Position == gl_in[1].gl_Position){
                if (v_in[0].left_ctrl == vec2(ZERO, ZERO)){
                    patch_out.type = START_CAP;
                    if(cap_style == CAP_STYLE_ROUND){
                        float radius = HALF_SQRT2 + (stroke_width / 2.0);
                        tessel_x = radius_to_tesselation(radius) / 2.;
                    }
                } else if (v_in[1].right_ctrl == vec2(ZERO, ZERO)){
                    patch_out.type = END_CAP;
                    if(cap_style == CAP_STYLE_ROUND){
                        float radius = HALF_SQRT2 + (stroke_width / 2.0);
                        tessel_x = radius_to_tesselation(radius) / 2.;
                    }
                }
                else {
                    patch_out.type = JOINT;
                    if(joint_style == JOINT_STYLE_ROUND){
                        float angle = acos(dot(patch_out.ctrl1_direction, -patch_out.ctrl2_direction));
                        float radius = HALF_SQRT2 + (stroke_width / 2.0);
                        tessel_x = radius_to_tesselation(radius) * (angle / PI*2.);
                    } else if (joint_style == JOINT_STYLE_MITER){
                        tessel_x = 4.;
                        // TODO: this could be 3 for a miter below miter distance
                    }
                }
            }

            // segment
            else {
                // TODO: do something smart here for adaptive line tesselation
                tessel_x = 64.;
            }

            // apply tesselation
            gl_TessLevelInner[0] = tessel_x;
            gl_TessLevelInner[1] = tessel_y;

            gl_TessLevelOuter[0] = tessel_y;
            gl_TessLevelOuter[1] = tessel_x;
            gl_TessLevelOuter[2] = tessel_y;
            gl_TessLevelOuter[3] = tessel_x;
        }
    }

    // some lines don't need an additional antialiasing border
    if(patch_out.type == STROKE
            && mod(stroke_width, 2.) == 1.                                        // odd, integral width
            && (START_VERTEX.x == END_VERTEX.x || START_VERTEX.y == END_VERTEX.y) // horizontal or vertical
            && (fract(START_VERTEX + vec2(.5)) == vec2(0))){                      // through pixel centers
        patch_out.aa_width = 0.;
    } else {
        patch_out.aa_width = HALF_SQRT2;
    }
}
