#version 320 es

precision highp float;

out vec2 uv;

const float ZEROf = 0.0f;
const float ONEf = 1.0f;
const float TWOf = 2.0f;
const int ONEi = 1;
const int TWOi = 2;

//
// This vertex shader fill the complete screen with a single triangle (which is clipped to a quad for free),without the
// use of vertex buffers. The idea is similar to the one described (for Vulkan) in:
//     https://www.saschawillems.de/?page_id=2122
//
// After activating a Pipeline with this shader in the vertex stage, draw the quad using:
//     glDrawArrays(GL_TRIANGLES, 0, 3)
//
void main() {
    uv = vec2((gl_VertexID << ONEi) & TWOi, gl_VertexID & TWOi);
    gl_Position = vec4(uv * TWOf + -ONEf, ZEROf, ONEf);
}
