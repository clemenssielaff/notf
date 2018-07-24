#version 320 es

precision highp float;

layout (quads, fractional_even_spacing) in;

patch in PatchData {
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
    mediump mat3 line_xform;
    mediump vec2 line_size;
    mediump vec2 position;
    mediump vec2 tex_coord;
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

#define START (gl_in[0].gl_Position.xy)
#define END   (gl_in[1].gl_Position.xy)

void calculate_line() 
{
    vec2 delta = END - START;
    float dist = length(delta);
    vec2 offset = normalize(delta);
    
    // move start and end coordinates to the bottom left of the stroke
    vec2 blub = vec2(offset.y, -offset.x) * stroke_width * HALF;
    vec2 start = START + blub;
    
    // calculate the transformation of points in screen space to line-space
    float angle = atan(delta.y, delta.x);
    float c = cos(angle);
    float s = sin(angle);

    mat3 transform = mat3(
        1., 0., start.x, 
        0., 1., start.y,
        0., 0., 1.);
    mat3 rotation = mat3(
        +c, -s, 0., 
        +s, +c, 0.,
        0., 0., 1.);

    v_out.line_xform = inverse(rotation * transform);

    v_out.line_size.x = dist;
    v_out.line_size.y = stroke_width;
}

void main()
{
    v_out.tex_coord.x = gl_TessCoord.x;
    v_out.tex_coord.y = gl_TessCoord.y;

    if(patch_data.type == CONVEX){
        // v_out.tex_coord.y = aa_width == 0.0 ? 1.0 : ONE - step(0.9, gl_TessCoord.y);

        // This always creates a triangle with zero area :/ but I hope that the GPU is quick to discard such polygons.
        // The alternative would be that I always pass 3 vertices to a patch, which would mean that I always pass an unused
        // vertex for each line segment, which might be slower
        vec2 delta = mix(END, START, gl_TessCoord.x) - base_vertex;
        // v_out.position = fma(vec2(step(HALF, gl_TessCoord.y) * (length(delta) - (sign(v_out.tex_coord.y - HALF) * aa_width))),
        //                      normalize(delta), base_vertex);
        v_out.position = fma(vec2(step(HALF, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (patch_data.type == CONCAVE) {
        // see comment in the CONVEX branch
        vec2 delta = mix(END, START, gl_TessCoord.x) - base_vertex;
        v_out.position = fma(vec2(step(HALF, gl_TessCoord.y) * length(delta)), normalize(delta), base_vertex);
    }

    else if (patch_data.type == TEXT) {
        vec2 uv_max = START + (glyph_max_corner - glyph_min_corner);
        v_out.tex_coord = mix(START, uv_max, vec2(gl_TessCoord.x, ONE - gl_TessCoord.y)) / atlas_size;
        v_out.position = mix(glyph_min_corner, glyph_max_corner, gl_TessCoord.xy);
    }

    else { // patch_data.type is some form of line
        float normal_offset = (aa_width + (stroke_width / 2.0)) * sign(gl_TessCoord.y - HALF);

        if(patch_data.type == STROKE){
            vec2 TVEC = vec2(ONE, gl_TessCoord.x);

            // bezier control points
            vec2 ctrl1 = START + (patch_data.ctrl1_direction * patch_data.ctrl1_length);
            vec2 ctrl2 = END + (patch_data.ctrl2_direction * patch_data.ctrl2_length);

            // screen space position of the vertex
            vec2 normal = normalize(mat3x2(ctrl1-START, ctrl2-ctrl1, END-ctrl2) * DERIV * (TVEC.xyy * TVEC.xxy)).yx * vec2(-ONE, ONE);
            v_out.position =
                    (mat4x2(START, ctrl1, ctrl2, END) * BEZIER * (TVEC.xyyy * TVEC.xxyy * TVEC.xxxy)).xy    // along spline
                    + (normal * normal_offset);                                                             // along normal
        }

        else if(patch_data.type == JOINT){
            vec2 normal;
            if(gl_TessCoord.x == ZERO){
                normal = (-patch_data.ctrl1_direction).yx * vec2(-ONE, ONE);
            } else {
                normal = (-patch_data.ctrl2_direction).yx * vec2(ONE, -ONE);
            }
            v_out.position = START + (normal * normal_offset);

            v_out.tex_coord.x = HALF;
        }

        else if(patch_data.type == START_CAP){
            // screen space position of the vertex
            vec2 normal = patch_data.ctrl1_direction.yx * vec2(-ONE, ONE);
            v_out.position = START
                    + (patch_data.ctrl1_direction * ((gl_TessCoord.x - ONE) * aa_width * TWO))  // along spline
                    + (normal * normal_offset);                                                 // along normal

            // cap texture coordinates
            v_out.tex_coord.x = ZERO;
            // v_out.tex_coord.y = (gl_TessCoord.x == ONE) ? v_out.tex_coord.y : ZERO;
        }

        else if(patch_data.type == END_CAP){
            // screen space position of the vertex
            vec2 normal = patch_data.ctrl2_direction.yx * vec2(ONE, -ONE);
            v_out.position = END
                    + (patch_data.ctrl2_direction * (-gl_TessCoord.x * aa_width * TWO)) // along spline
                    + (normal * normal_offset);                                         // along normal

            // cap texture coordinates
            v_out.tex_coord.x = ONE;
            // v_out.tex_coord.y = gl_TessCoord.x > 0.9 ? ZERO : v_out.tex_coord.y;
        }
    }

    // clip-space position of the vertex
    gl_Position = projection * vec4(v_out.position, -ONE, ONE);

    // patch type
    v_out.patch_type = patch_data.type;

    calculate_line();
}
