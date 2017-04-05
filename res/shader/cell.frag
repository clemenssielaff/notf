// header, required to read the file into NoTF at compile time (you can comment it out while working on the file though)
R"=====(
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif

layout(std140) uniform frag {
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
    int texType;
    int type;
};
uniform sampler2D tex;
in vec2 ftcoord;
in vec2 fpos;
out vec4 outColor;


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
        return min(1.0, (1.0-abs(ftcoord.x*2.0-1.0))*strokeMult) * min(1.0, ftcoord.y);
    }
#endif

void main(void) {
    vec4 result;
    float scissor = scissorMask(fpos);

#ifdef GEOMETRY_AA
    float strokeAlpha = strokeMask();
    if (strokeAlpha < strokeThr) {
        discard;
        // this really slows things down, but you can see (tiny) artefacts in the rotating angles when not set
        // either find another way to fix the artefacts or provide a flag to enable this feature, as it is not used
        // for anything else than transparent, overlapping strokes - but slows down everything significantly
    }
#else
    float strokeAlpha = 1.0;
#endif

    // Gradient
    if (type == 0) {
        // Calculate gradient color using box gradient
        vec2 pt = (paintMat * vec3(fpos,1.0)).xy;
        float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);
        vec4 color = mix(innerCol,outerCol,d);

        // Combine alpha
        color *= strokeAlpha * scissor;
        result = color;
    }

    // Image
    else if (type == 1) {
        // Calculate color from texture
        vec2 pt = (paintMat * vec3(fpos,1.0)).xy / extent;
        vec4 color = texture(tex, pt);
        if (texType == 1) {
            color = vec4(color.xyz*color.w,color.w);
        }
        else if (texType == 2) {
            color = vec4(color.x);
        }

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

    // return result
    outColor = result;
}
//)====="; // footer, required to read the file into NoTF at compile time
