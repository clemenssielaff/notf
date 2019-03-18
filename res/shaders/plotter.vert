#version 320 es

precision highp float;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_left_ctrl;
layout(location = 2) in vec2 a_right_ctrl;
layout(location = 3) in mat2x3 a_instance_xform;

out VertexData {
    vec2 left_ctrl;
    vec2 right_ctrl;
} v_out;

void main(){
    // all plotter vertices are positioned with integer values in the center of the pixels
    // the first visible pixel has the index 1, meaning all positions are shifted 0.5 pixels to the left and bottom
    //
    //   || ... | ... | ... |
    //    +-----+-----+-----+-
    //   || x=1 | x=2 | x=3 | ...
    //   || y=3 | y=3 | y=3 | ...
    //    +-----+-----+-----+-
    //   || x=1 | x=2 | x=3 | ...
    //   || y=2 | y=2 | y=2 | ...
    //    +-----+-----+-----+-
    //   || x=1 | x=2 | x=3 | ...
    //   || y=1 | y=1 | y=1 | ...
    //    +=====+=====+=====+=
    // origin
    //
    gl_Position = vec4(a_position.xy - vec2(.5), 0, 1);

    // pass attributes into block
    v_out.left_ctrl = a_left_ctrl;
    v_out.right_ctrl = a_right_ctrl;
}
