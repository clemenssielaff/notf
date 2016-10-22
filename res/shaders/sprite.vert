#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 projection;

void main()
{
    TexCoords = vertex.zw;
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
}

//#extension GL_ARB_explicit_attrib_location : require
//#extension GL_ARB_explicit_uniform_location : require

//layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

//out vec2 tex_coords;

//uniform mat4 model;
//uniform mat4 projection;

//void main()
//{
//    tex_coords = vertex.zw;
//    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
//}
