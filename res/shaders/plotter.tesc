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
uniform float aa_width;

patch out PatchData {
    vec2 start;
    vec2 end;
    float ctrl1_length;
    float ctrl2_length;
    vec2 ctrl1_direction;
    vec2 ctrl2_direction;
    int type;
} patch_data;

// constant symbols
const float ZERO = 0.0f;
const float ONE = 1.0f;
const float TWO = 2.0f;
const float THREE = 3.0f;

// patch types
const int CONVEX    = 1;
const int CONCAVE   = 2;
const int STROKE    = 3;
const int TEXT      = 4;
const int JOINT     = 31;
const int START_CAP = 32;
const int END_CAP   = 33;

#define START_VERTEX (gl_in[0].gl_Position.xy)
#define END_VERTEX   (gl_in[1].gl_Position.xy)

// settings
const float tessel_x_factor = 0.0001;
const float tessel_x_max = 64.0;

void main(){

    // vertex position pass-through
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;

    // we only need to calculate the patch data once
    if (ID != 0){
        return;
    }

    patch_data.type = patch_type;

    if(patch_type == TEXT) {
        gl_TessLevelInner[0] = ZERO;
        gl_TessLevelInner[1] = ZERO;

        gl_TessLevelOuter[0] = ONE;
        gl_TessLevelOuter[1] = ONE;
        gl_TessLevelOuter[2] = ONE;
        gl_TessLevelOuter[3] = ONE;

        // this holds the position of the glyph's min and max corners
        patch_data.ctrl1_direction = v_in[0].left_ctrl;
        patch_data.ctrl2_direction = v_in[0].right_ctrl;
    }

    // branch for everything but text
    else {
        // direction of the control points from their nearest vertex
        patch_data.ctrl1_direction = normalize(v_in[0].right_ctrl);
        patch_data.ctrl2_direction = normalize(v_in[1].left_ctrl);

        // start and end vertex are moved outwards along their tangent
        // this is to ensure that the line is long enough to cover all pixels involved
        float lengthwise_offset = aa_width + (stroke_width * 0.5);
        patch_data.start = START_VERTEX - (lengthwise_offset * patch_data.ctrl1_direction);
        patch_data.end = END_VERTEX - (lengthwise_offset * patch_data.ctrl2_direction);

        // ctrl point delta magnitude
        patch_data.ctrl1_length = max(ZERO, length(v_in[0].right_ctrl) - ONE);
        patch_data.ctrl2_length = max(ZERO, length(v_in[1].left_ctrl) - ONE);

        // bezier spline points
        vec2 ctrl1 = START_VERTEX + (patch_data.ctrl1_direction * patch_data.ctrl1_length);
        vec2 ctrl2 = END_VERTEX + (patch_data.ctrl2_direction * patch_data.ctrl2_length);

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

            // segment sub-types
            if(gl_in[0].gl_Position == gl_in[1].gl_Position){
                if (v_in[0].left_ctrl == vec2(ZERO, ZERO)){
                    patch_data.type = START_CAP;
                } else if (v_in[1].right_ctrl == vec2(ZERO, ZERO)){
                    patch_data.type = END_CAP;
                } else {
                    patch_data.type = JOINT;
                }
            }

            // segment
            else {
                // vec2 line = END - START;
                // float line_dot = dot(line, line);
                // tessel_x = min(tessel_x_max, ONE +
                //                 floor(length(line) * tessel_x_factor *
                //                         (length(START + line * (dot(ctrl1 - START, line) / line_dot) - ctrl1) +
                //                             length(START + line * (dot(ctrl2 - START, line) / line_dot) - ctrl2))));
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
}
