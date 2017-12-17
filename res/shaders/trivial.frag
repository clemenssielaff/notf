#version 320 es

precision mediump float;

layout(location=0) out vec4 f_color;

const float ONE = 1.0f;

void main() {
  f_color = vec4(ONE, ONE, ONE, ONE);
}
