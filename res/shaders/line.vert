#version 320 es

precision highp float;

layout(location = 0) in vec2 a_position;

void main(){
    gl_Position = vec4(a_position.xy, -1.f, 1.f);
}
