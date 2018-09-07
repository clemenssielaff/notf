#version 320 es

precision highp float;

layout(location = 0) in vec3 a_position;

uniform mat4 projection, modelview;

void main(){
    gl_Position = projection * modelview * vec4(a_position, 1.f);
}
