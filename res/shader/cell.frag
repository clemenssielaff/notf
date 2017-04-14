// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

layout(std140) uniform variables {
    mat3 scissorMat;
    mat3 paintMat;
    vec4 innerCol;
    vec4 outerCol;
    vec2 scissorExt;
    vec2 scissorScale;
    vec2 extent;
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    int type;
};
uniform sampler2D image;
in vec2 tex_coord;
in vec2 vertex_pos;
out vec4 result; // fragment color


float sdroundrect(vec2 pt, vec2 ext, float rad) {
    vec2 ext2 = ext - vec2(rad,rad);
    vec2 d = abs(pt) - ext2;
    return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;
}

// Scissoring
float scissorMask(vec2 p) {
    vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
    sc = vec2(0.5,0.5) - sc * scissorScale;
    return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);
}

#ifdef GEOMETRY_AA
    // Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
    float strokeMask() {
        return min(1.0, (1.0-abs(tex_coord.x*2.0-1.0))*strokeMult) * min(1.0, tex_coord.y);
    }
#endif

void main(void) {
    float scissor = scissorMask(vertex_pos);

#ifdef GEOMETRY_AA
    float strokeAlpha = strokeMask();
#else
    float strokeAlpha = 1.0;
#endif

#ifdef SAVE_ALPHA_STROKE
    if (strokeAlpha < strokeThr) {
        discard;
    }
#endif

    // Gradient
    if (type == 0) {
        // Calculate gradient color using box gradient
        vec2 pt = (paintMat * vec3(vertex_pos,1.0)).xy;
        float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
        vec4 color = mix(innerCol,outerCol,d);

        // Combine alpha
        color *= strokeAlpha * scissor;
        result = color;
    }

    // Image
    else if (type == 1) {
        // Calculate color from texture
        vec2 pt = (paintMat * vec3(vertex_pos,1.0)).xy / extent;
        vec4 color = texture(image, pt);

        // Apply color tint and alpha.
        color *= innerCol;

        // Combine alpha
        color *= strokeAlpha * scissor;
        result = color;
    }

    // Stencil fill
    else if (type == 2) {
        result = vec4(1,1,1,1);
    }

    // Text
    else if (type == 3) {
        result = vec4(1, 1, 1, texture(image, tex_coord).r) * innerCol;
    }

    // Error
    else {
        result = vec4(0,0,0,1);
    }
}
//)====="; // footer, required to read the file into NoTF at compile time
