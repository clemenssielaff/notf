// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

uniform mat4 projection_matrix;
in vec2 vertex;
in vec2 uv;
out vec2 tex_coord;
out vec2 vertex_pos;

void main(void) {
    tex_coord = uv;
    vertex_pos = vertex;
    gl_Position = projection_matrix * vec4(vertex, 0, 1);
}
//)====="; // footer, required to read the file into NoTF at compile time
