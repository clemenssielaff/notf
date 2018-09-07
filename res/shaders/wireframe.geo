#version 320 es

precision highp float;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VertexData {
    vec2 position;
    mediump vec3 patch_distance;
} v_in[];

out VertexData {
    mediump vec2 position;
    mediump vec3 tri_distance;
    mediump vec3 patch_distance;
} v_out;

const float ZERO = 0.f;

float height(vec2 p, vec2 a, vec2 b)
{
    return abs(((b.y - a.y) * p.x) - ((b.x - a.x) * p.y) + (b.x * a.y) - (b.y * a.x)) / length(a - b);
}

void main()
{
    v_out.position = v_in[0].position;
    v_out.tri_distance = vec3(height(v_in[0].position, v_in[1].position, v_in[2].position), ZERO, ZERO);
    v_out.patch_distance = v_in[0].patch_distance;

    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    v_out.position = v_in[1].position;
    v_out.tri_distance = vec3(ZERO, height(v_in[1].position, v_in[0].position, v_in[2].position), ZERO);
    v_out.patch_distance = v_in[1].patch_distance;

    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    v_out.position = v_in[2].position;
    v_out.tri_distance = vec3(ZERO, ZERO, height(v_in[2].position, v_in[0].position, v_in[1].position));
    v_out.patch_distance = v_in[2].patch_distance;

    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
