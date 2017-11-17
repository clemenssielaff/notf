#version 320 es

precision highp float;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_normal;
layout(location = 2) in vec2 a_texcoord;
layout(location = 3) in mat4 i_xform;

uniform mat4 projection, modelview, normalMat;

out mediump vec3 v_position;
out mediump vec3 v_normal;
out mediump vec2 v_texcoord;

void main(){
    gl_Position = projection * i_xform * modelview * vec4(a_position.xyz, 1.f);
    
    vec4 vertPos4 = modelview * vec4(a_position.xyz, 1.f);
    v_position = vec3(vertPos4) / vertPos4.w;
    
    v_normal = vec3(normalMat * a_normal);
    
    v_texcoord = a_texcoord;
}
