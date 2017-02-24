// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#version 300 es
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

in vec4 coord;
out vec2 tex_coord;

void main()
{
    gl_Position = vec4(coord.xy, 0, 1);
    tex_coord = coord.zw;
}

//)====="; // footer, required to read the file into NoTF at compile time
