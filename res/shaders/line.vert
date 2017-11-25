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
    gl_Position = vec4(a_position.xy, 0.0, 0.0);

    // pass attributes into block
    v_out.first_ctrl = a_first_ctrl;
    v_out.second_ctrl = a_second_ctrl;
}
