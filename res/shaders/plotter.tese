#version 320 es

precision highp float;

layout (quads, fractional_even_spacing) in;

patch in PatchData {
    vec2 start;
    vec2 end;
    float ctrl1_length;
    float ctrl2_length;
    vec2 ctrl1_direction;
    vec2 ctrl2_direction;
    int type;
} patch_data;
#define glyph_min_corner (patch_data.ctrl1_direction)
#define glyph_max_corner (patch_data.ctrl2_direction)

uniform float stroke_width;
uniform mat4 projection;
uniform float aa_width;

uniform vec2 vec2_aux1;
#define base_vertex vec2_aux1
#define atlas_size  vec2_aux1

out VertexData {
    mediump mat3x2 screen_to_line_xform;
    mediump vec2 line_size;
    mediump vec2 texture_coord;
    mediump flat int patch_type;
} v_out;

// constant symbols
const float ZERO    = 0.0f;
const float HALF    = 0.5f;
const float ONE     = 1.0f;
const float TWO     = 2.0f;
const float THREE   = 3.0f;
const float SIX     = 6.0f;

const mat4 BEZIER = mat4(
     ONE,    ZERO,   ZERO,  ZERO,
    -THREE,  THREE,  ZERO,  ZERO,
     THREE, -SIX,    THREE, ZERO,
    -ONE,    THREE, -THREE, ONE);

const mat3 DERIV = mat3(
     THREE,  ZERO, ZERO,
    -SIX,    SIX,  ZERO,
     THREE, -SIX,  THREE);

 // patch types
const int CONVEX    = 1;
const int CONCAVE   = 2;
const int STROKE    = 3;
const int TEXT      = 4;
const int JOINT     = 31;
const int START_CAP = 32;
const int END_CAP   = 33;

#define START_VERTEX (gl_in[0].gl_Position.xy)
#define END_VERTEX   (gl_in[1].gl_Position.xy)

void calculate_line()
{
    // Calculate line information.
    // Note that we are not using path_data.start/end here because those denote the start and end of the polygon
    // enclosing the line. Instead, we want the actual line that determines how much a fragment is activated.
    vec2 delta = END_VERTEX - START_VERTEX;
    float line_length = length(delta);
    v_out.line_size.x = line_length + stroke_width;
    v_out.line_size.y = stroke_width;

    // calculate the transformation of points in screen space to line-space
    float angle = -atan(delta.y, delta.x);
    float c = cos(angle);
    float s = sin(angle);
    float offset = stroke_width * HALF;
    v_out.screen_to_line_xform = mat3x2(
        vec2(c, s),
        vec2(-s, c),
        vec2(offset - (c * START_VERTEX.x) + (s * START_VERTEX.y),
             offset - (c * START_VERTEX.y) - (s * START_VERTEX.x))
    );
}

void main()
{
    vec2 vertex_pos;
    v_out.texture_coord = gl_TessCoord.xy;

    if(patch_data.type == CONVEX){
        // v_out.texture_coord.y = aa_width == 0.0 ? 1.0 : ONE - step(0.9, gl_TessCoord.y);

        // This always creates a triangle with zero area :/ but I hope that the GPU is quick to discard such polygons.
        // The alternative would be that I always pass 3 vertices to a patch, which would mean that I always pass an unused
        // vertex for each line segment, which might be slower
        vec2 delta = mix(patch_data.end, patch_data.start, gl_TessCoord.x) - base_vertex;
        // vertex_pos = fma(vec2(step(HALF, gl_TessCoord.y) * (length(delta) - (sign(v_out.texture_coord.y - HALF) * aa_width))),
        //                      normalize(delta), base_vertex);
        vertex_pos = fma(vec2(step(HALF, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (patch_data.type == CONCAVE) {
        // see comment in the CONVEX branch
        vec2 delta = mix(patch_data.end, patch_data.start, gl_TessCoord.x) - base_vertex;
        vertex_pos = fma(vec2(step(HALF, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (patch_data.type == TEXT) {
        vec2 uv_max = START_VERTEX + (glyph_max_corner - glyph_min_corner);
        v_out.texture_coord = mix(START_VERTEX, uv_max, vec2(gl_TessCoord.x, ONE - gl_TessCoord.y)) / atlas_size;
        vertex_pos = mix(glyph_min_corner, glyph_max_corner, gl_TessCoord.xy);
    }

    else { // patch_data.type is some form of line
        float normal_offset = (aa_width + (stroke_width / 2.0)) * sign(gl_TessCoord.y - HALF);

        if(patch_data.type == STROKE){
            vec2 TVEC = vec2(ONE, gl_TessCoord.x);

            // bezier control points
            vec2 ctrl1 = START_VERTEX + (patch_data.ctrl1_direction * patch_data.ctrl1_length);
            vec2 ctrl2 = END_VERTEX + (patch_data.ctrl2_direction * patch_data.ctrl2_length);

            // screen space position of the vertex
            vec2 normal = normalize(mat3x2(ctrl1-patch_data.start, ctrl2-ctrl1, patch_data.end-ctrl2) * DERIV * (TVEC.xyy * TVEC.xxy)).yx * vec2(-ONE, ONE);
            vertex_pos =
                    (mat4x2(patch_data.start, ctrl1, ctrl2, patch_data.end) * BEZIER * (TVEC.xyyy * TVEC.xxyy * TVEC.xxxy)).xy    // along spline
                    + (normal * normal_offset);                                                             // along normal
        }

        else if(patch_data.type == JOINT){
            vec2 normal;
            if(gl_TessCoord.x == ZERO){
                normal = (-patch_data.ctrl1_direction).yx * vec2(-ONE, ONE);
            } else {
                normal = (-patch_data.ctrl2_direction).yx * vec2(ONE, -ONE);
            }
            vertex_pos = patch_data.start + (normal * normal_offset);

            v_out.texture_coord.x = HALF;
        }

        else if(patch_data.type == START_CAP){
            // screen space position of the vertex
            vec2 normal = patch_data.ctrl1_direction.yx * vec2(ONE, -ONE);
            vertex_pos = patch_data.start
                    + (patch_data.ctrl1_direction * ((gl_TessCoord.x - ONE) * aa_width * TWO))  // along spline
                    + (normal * normal_offset);                                                 // along normal

            // cap texture coordinates
            v_out.texture_coord.x = ZERO;
            // v_out.texture_coord.y = (gl_TessCoord.x == ONE) ? v_out.texture_coord.y : ZERO;
        }

        else if(patch_data.type == END_CAP){
            // screen space position of the vertex
            vec2 normal = patch_data.ctrl2_direction.yx * vec2(-ONE, ONE);
            vertex_pos = patch_data.end
                    + (patch_data.ctrl2_direction * (-gl_TessCoord.x * aa_width * TWO)) // along spline
                    + (normal * normal_offset);                                         // along normal

            // cap texture coordinates
            v_out.texture_coord.x = ONE;
            // v_out.texture_coord.y = gl_TessCoord.x > 0.9 ? ZERO : v_out.texture_coord.y;
        }
    }

    // clip-space position of the vertex
    gl_Position = projection * vec4(vertex_pos, -ONE, ONE);

    // patch type
    v_out.patch_type = patch_data.type;

    calculate_line();
}
