// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#version 300 es
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

uniform sampler2D tex;
uniform vec4 color;
in vec2 tex_coord;
out vec4 frag_color;

void main()
{
    frag_color = vec4(1, 1, 1, texture2D(tex, tex_coord).r) * color;
//    frag_color = vec4(0, 0, 0, 1 );
}

//)====="; // footer, required to read the file into NoTF at compile time
