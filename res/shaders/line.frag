#version 320 es

precision mediump float;

in vec3 g_tri_distance;
in vec3 g_patch_distance;

layout(location=0) out vec4 f_color;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0.0f, 1.0f);
    d = 1.0f - exp2(-2.0f*d*d);
    return d;
}

void main() {
    vec3 color = vec3(0.5f, 0.5f, 0.5f);
    float d1 = min(min(g_tri_distance.x, g_tri_distance.y), g_tri_distance.z);
    float d2 = min(min(g_patch_distance.x, g_patch_distance.y), g_patch_distance.z);
    color = amplify(d1, 80.0f, -0.5f) * amplify(d2, 120.0f, -0.5f) * color;
    f_color = vec4(color, 1.0f);
}
