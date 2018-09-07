#version 320 es

precision highp float;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_first_ctrl;
layout(location = 2) in vec2 a_second_ctrl;

out VertexData {
    vec2 first_ctrl;
    vec2 second_ctrl;
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
    gl_Position = vec4(a_position.xy - vec2(0.5, 0.5), 0.0, 0.0);

    // pass attributes into block
    v_out.first_ctrl = a_first_ctrl;
    v_out.second_ctrl = a_second_ctrl;
}
