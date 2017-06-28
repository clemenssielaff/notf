// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

layout(std140) uniform variables {
    vec4 paintRot;
    vec4 scissorRot;
    vec2 paintTrans;
    vec2 scissorTrans;
    vec2 scissorExt;
    vec2 scissorScale;
    vec4 innerCol;
    vec4 outerCol;
    vec2 extent;
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    int type;
    float _padding;
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
    mat3x2 scissorMat = mat3x2(scissorRot, scissorTrans);
    vec2 sc = (abs((scissorMat * vec3(p,1.0)).xy) - scissorExt);
    sc = vec2(0.5,0.5) - sc * scissorScale;
    return clamp(sc.x,0.,1.) * clamp(sc.y,0.,1.);
}

#ifdef GEOMETRY_AA
    // Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
    float strokeMask() {
        return min(1.0, (1.0-abs(tex_coord.x*2.0-1.0))*strokeMult) * min(1.0, tex_coord.y);
    }
#endif

void main(void) {
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

    mat3x2 paintMat = mat3x2(paintRot, paintTrans);

    // Gradient
    if (type == 0) {
        // Calculate gradient color using box gradient
        vec2 pt = (paintMat * vec3(vertex_pos, 1.0)).xy;
        float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
        result = mix(innerCol,outerCol,d) * strokeAlpha;
    }

    // Image
    else if (type == 1) {
        // Calculate color from texture
        vec2 pt = (paintMat * vec3(vertex_pos, 1.0)).xy / extent;
        result = texture(image, pt) * innerCol * strokeAlpha;
    }

    // Stencil fill
    else if (type == 2) {
        result = vec4(1, 1, 1, 1);
    }

    // Text
    else if (type == 3) {
        result = vec4(1, 1, 1, texture(image, tex_coord).r) * innerCol;
    }

    // Error
    else {
        result = vec4(0, 0, 0, 1);
    }

    // apply scissor
    result *= scissorMask(vertex_pos);
}
//)====="; // footer, required to read the file into NoTF at compile time
