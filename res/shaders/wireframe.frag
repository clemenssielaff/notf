#version 320 es

precision mediump float;

in VertexData {
    vec2 position;
    vec3 tri_distance;
    vec3 patch_distance;
} v_in;

layout(location=0) out vec4 f_color;

const float PATCH_LINE_WIDTH = 0.5f;
const float TRI_LINE_WIDTH = 0.25f;
const float LINE_AA = 0.75f;

const float TRI_WEIGHT = 0.2f;
const float PATCH_WEIGHT = 1.0f;

const float ZERO = 0.0f;
const float ONE = 1.0f;

void main() {
    vec3 color = vec3(v_in.position / 800.f, ZERO);

    float tri_dist = min(min(v_in.tri_distance.x, v_in.tri_distance.y), v_in.tri_distance.z);
    float patch_dist = min(min(v_in.patch_distance.x, v_in.patch_distance.y), v_in.patch_distance.z);

    float tri_offset = (ONE - smoothstep(TRI_LINE_WIDTH, TRI_LINE_WIDTH + LINE_AA, tri_dist)) * TRI_WEIGHT;
    float patch_offset = (ONE - smoothstep(PATCH_LINE_WIDTH, PATCH_LINE_WIDTH + LINE_AA, patch_dist)) * PATCH_WEIGHT;

    color = min(vec3(ONE, ONE, ONE), color + tri_offset + patch_offset);

    f_color = vec4(color, ONE);
}
