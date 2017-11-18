#version 320 es

precision highp float;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 te_patch_distance[3];

out mediump vec3 g_tri_distance;
out mediump vec3 g_patch_distance;

void main()
{
    g_patch_distance = te_patch_distance[0];
    g_tri_distance = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    g_patch_distance = te_patch_distance[1];
    g_tri_distance = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    g_patch_distance = te_patch_distance[2];
    g_tri_distance = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
