// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform vec2 viewSize;
in vec2 vertex;
in vec2 tcoord;
out vec2 ftcoord;
out vec2 fpos;

void main(void) {
    ftcoord = tcoord;
    fpos = vertex;
    gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 1.0 - 2.0*vertex.y/viewSize.y, 0, 1);
}
//)====="; // footer, required to read the file into NoTF at compile time
