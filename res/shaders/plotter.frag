#version 320 es

precision mediump float;

// constants ======================================================================================================== //

// patch types
const int CONVEX    = 1;
const int CONCAVE   = 2;
const int TEXT      = 3;
const int STROKE    = 4;
const int JOINT     = 41;
const int START_CAP = 42;
const int END_CAP   = 43;

// cap styles
const int CAP_STYLE_BUTT = 1;
const int CAP_STYLE_ROUND = 2;
const int CAP_STYLE_SQUARE = 3;

// joint styles
const int JOINT_STYLE_MITER = 1;
const int JOINT_STYLE_ROUND = 2;
const int JOINT_STYLE_BEVEL = 3;

// uniforms ======================================================================================================== //

uniform sampler2D font_texture;

layout(std140) uniform PaintBlock {
    vec4 paint_rotation;    //  0 (size = 4)
    vec2 paint_translation; //  4 (size = 2)
    vec2 paint_size;        //  6 (size = 2)
    vec4 clip_rotation;     //  8 (size = 4)
    vec2 clip_translation;  // 12 (size = 2)
    vec2 clip_size;         // 14 (size = 2)
    vec4 inner_color;       // 16 (size = 4)
    vec4 outer_color;       // 20 (size = 4)
    int type;               // 24 (size = 1)
    float stroke_width;     // 25 (size = 1)
    float radius;           // 26 (size = 1)
    float feather;          // 27 (size = 1)
};

// pipeline ======================================================================================================== //

in FragmentData {
    /// start point of the line in screen coordinates
    flat vec2 line_origin;

    /// normalized vector in direction of the line
    flat vec2 line_direction;

    /// x = line width, y = line length
    flat vec2 line_size;

    /// transformation of the origin from screen- to line-space
    flat mat3x2 line_xform;

    /// texture coordinate of this fragment
    vec2 texture_coord;

    /// type of patch creating this fragment
    flat int patch_type;
} frag_in;

layout(location=0) out vec4 final_color;

// general ========================================================================================================= //

/// Cross product for 2D vectors.
float cross2(vec2 a, vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

// perfect antialiasing ============================================================================================ //

/// Calculates the coverage of a 1x1 square (a pixel) that is intersected by a half-space at a given angle.
/// For the area, we need the covered polygon which can have 3-5 sides, depending on the angle and offset of
/// intersection. In order to speed up calculation on a GPU what we do is a bit more involved:
///     1. Allocate 8 vec2s and define them to (-1, -1). These are going to be our vertices in screen coordinates.
///        Values less than (0,0) can be used safely to denote "undefined" values as a fragment with negative
///        coordinates would never be generated in the first place.
///     2. Check each corner of the pixel and if it is inside the half space, write its position into the
///        corresponding vertex. Corner vertices are at indices [0, 2, 4, 6]. If the corner is outside the half space,
///        ignore it.
///        Also remember the index of the first corner vertex that is inside the half space as we need it later.
///     3. Find the intersection of each edge of the pixel with the half space.
///        If the intersection exists and falls onto the edge of the pixel, store the intersection as a new vertex.
///        Intersection vertices are stored at indices [1, 3, 5, 7].
///     4. Fill the remaining, undefined vertices by copying their nearest defined neighbor. It doesn't matter if the
///        neighbor has a higher or lower index, all that matters is that in the end, all are defined and that filling
///        in does not add any area to the polygon.
///     5. Lasty, calculate and return the area of the polygon.
float half_space_coverage(vec2 point, vec2 start, vec2 direction, float half_stroke_width)
{
    vec2 corners[4] = vec2[](
        point + vec2(+0.5, +0.5),
        point + vec2(-0.5, +0.5),
        point + vec2(-0.5, -0.5),
        point + vec2(+0.5, -0.5)
    );
    vec2 vertices[8] = vec2[](vec2(-1), vec2(-1), vec2(-1), vec2(-1), vec2(-1), vec2(-1), vec2(-1), vec2(-1));

    // check for each corner of the pixel if it is inside the half space
    int first_known_vertex = 8;
    for(int i=0; i < 4; ++i){
        if(sign(cross2(direction, corners[i] - start)) < 0.){
            vertices[i*2] = corners[i];
            first_known_vertex = min(first_known_vertex, i*2);
        }
    }
    if(first_known_vertex == 8){
        return 0.; // if no corner is inside the half space, the coverage must be zero
    }

    // find all intersections of pixel edges with the half space
    for(int i=0; i < 4; ++i){
        vec2 edge = corners[(i+1) % 4] - corners[i];
        float det = cross2(edge, direction);
        if(det == 0.){
            continue;
        }
        float t = cross2(start - corners[i], direction) / det;
        if(0. <= t && t <= 1.){
            vertices[(i*2)+1] = corners[i] + (edge * t);
        }
    }

    // fill unknown vertices
    for(int i, index = first_known_vertex+1; i < 7; ++i, index = ++index % 8){
        if(vertices[index].x < 0.){
            vertices[index] = vertices[index == 0 ? 7 : index-1];
        }
    }

    // calculate the covered area
    float result = 0.;
    for(int i=0; i < 8; ++i){
        result += vertices[i].x * vertices[(i + 1) % 8].y;
        result -= vertices[i].y * vertices[(i + 1) % 8].x;
    }
    return result/2.;
}

/// Calculates the "perfect" coverage area of the fragment by an infinite line.
/// This method is probably (?) a lot more expensive than the sampling approach without much of a visual difference.
/// I keep it as a reference and because there might be a use-case for "perfect" line anti-aliasing in the future.
/// @param coord        Coordinate to sample (typically that is going to be `gl_FragCoord.xy`).
/// @param point        Any point on the line.
/// @param direction    Direction of the line (normalized).
/// @param width        Width of the line.
float perfect_sample(vec2 coord, vec2 point, vec2 direction, float width)
{
    const float half_diagonal = sqrt(2.) / 2.;
    float half_width = width / 2.;

    // early outs
    float distance_to_line = distance(coord, point + (direction * dot(coord-point, direction)));
    if(distance_to_line > half_diagonal + half_width){
        return 0.; // clearly outside
    }
    if(distance_to_line + half_diagonal < half_width){
        return 1.; // clearly inside
    }

    // calculate coverage of two half spaces and combine them into the pixel coverage
    vec2 normal = vec2(direction.y, -direction.x);
    return half_space_coverage(coord, point - (normal * half_width), direction, half_width)
          -half_space_coverage(coord, point + (normal * half_width), direction, half_width);
}

// fast antialiasing =============================================================================================== //

/// Sample pattern.
/// for a visual reference of the sample pattern see {notf_root}/dev/diagrams/aa_pattern.svg
const vec2 SAMPLE_PATTERN[4] = vec2[](
    vec2( 1,  5)  / 32.,
    vec2( 7, 13)  / 32.,
    vec2(11,  3)  / 32.,
    vec2(15,  9)  / 32.
);

/// We apply uniform weights across all sample points.
const vec4 SAMPLE_WEIGHTS = vec4(1);

/// The samples are rotated around the origin and applied once in each quadrant.
const vec2 SAMPLE_QUADRANTS[4] = vec2[4](vec2(+1, +1),  // top-right quadrant
                                         vec2(+1, -1),  // bottom-right quadrant
                                         vec2(-1, -1),  // bottom-left quadrant
                                         vec2(-1, +1)); // top-left quadrant

/// Cheap but precise super-sampling of the fragment coverage by the rendered line.
/// @param sample_x Whether or not to sample the start- and end- edges of the line (only true for caps).
/// @param sample_y Whether or not to sample the sides parallel to the center line (usually true).
float sample_line(vec2 coord, mat3x2 xform, vec2 size, bool sample_x, bool sample_y)
{
    // transform the coordinate from screen-space into line-space
    coord = (xform * vec3(coord, 1)).xy;

    float result = 0.;
    for(int quadrant = 0; quadrant < 4; ++quadrant){
        vec4 samples_x, samples_y;
        for(int i = 0; i < 4; ++i){
            vec2 sample_coord = fma(SAMPLE_PATTERN[i], SAMPLE_QUADRANTS[quadrant], coord);
            samples_x[i] = sample_coord.x;
            samples_y[i] = sample_coord.y;
        }
        samples_x = sample_x ? (step(0., samples_x) - step(size.x, samples_x)) : vec4(1);
        samples_y = sample_y ? (step(0., samples_y) - step(size.y, samples_y)) : vec4(1);
        result += dot(step(2., samples_x + samples_y), SAMPLE_WEIGHTS);
    }
    return result / 16.;
}

float sample_circle(vec2 coord, vec2 center, float radius_sq)
{
    float result = 0.;
    for(int quadrant = 0; quadrant < 4; ++quadrant){
        for(int i = 0; i < 4; ++i){
            vec2 sample_pos = coord + (SAMPLE_PATTERN[i] * SAMPLE_QUADRANTS[quadrant]);
            vec2 delta = sample_pos - center;
            result += (dot(delta, delta) <= radius_sq) ? 1. : 0.;
        }
    }
    return result / 16.;
}

// ================================================================================================================= //

/// Signed distance to a rounded rectangle centered at the origin, used for all gradients.
/// * Box gradients use the rounded rectangle as it is.
/// * Circle gradients use a rounded rectangle where size = (radius, radius).
/// * Linear gradients are set up to use only one edge of a very large rectangle.
/// *Thanks to Mikko Mononen for the function and helpful explanation!*
/// See https://github.com/memononen/nanovg/issues/369
/// @param point    The distance to this point is calculated.
/// @param size     Size of the rectangle.
/// @param radius   Radius of the rounded corners of the rectangle.
float get_rounded_rect_distance(vec2 point, vec2 size, float radius)
{
    vec2 inner_size = size - vec2(radius, radius);
    vec2 delta = abs(point) - inner_size;
    return min(max(delta.x, delta.y), 0.) + length(max(delta, 0.)) - radius;
}

/// Scissor factor from a rotated rectangle defined in the paint.
float get_clipping_factor()
{
    mat3x2 clip_xform = mat3x2(clip_rotation, clip_translation);
    vec2 clip = vec2(0.5, 0.5) - (abs((clip_xform * vec3(gl_FragCoord.xy, 1.)).xy) - clip_size);
    return clamp(clip.x, 0., 1.) * clamp(clip.y, 0., 1.);
}

// main ============================================================================================================ //

void main()
{
    if(get_clipping_factor() == 0.){
        discard;
    }

    float half_width = frag_in.line_size.y / 2.;

    vec3 color = vec3(1);
    float alpha = 0.;
    if(frag_in.patch_type == TEXT){
        color = vec3(1);
        alpha = texture(font_texture, frag_in.texture_coord).r;
    }
    else if(frag_in.patch_type == STROKE){
        color = vec3(0, 1, 0);
#ifdef SAMPLE_PERFECT
        alpha = perfect_sample(gl_FragCoord.xy, frag_in.line_origin, frag_in.line_direction, frag_in.line_size.y);
#else
        alpha = sample_line(gl_FragCoord.xy, frag_in.line_xform, frag_in.line_size, false, true);
#endif
    }
    else if(frag_in.patch_type == JOINT){
        color = vec3(1, .5, 0);
        alpha = sample_circle(gl_FragCoord.xy, frag_in.line_origin, half_width * half_width);
    }
    else if(frag_in.patch_type == START_CAP || frag_in.patch_type == END_CAP){
        color = vec3(0, .5, 1);
        alpha = sample_circle(gl_FragCoord.xy, frag_in.line_origin, half_width * half_width);
    }
    if(alpha == 0.){
        discard;
    }

    final_color = vec4(color, alpha * .5);
}
