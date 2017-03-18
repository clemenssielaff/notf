#pragma once

namespace notf {

/** HTML5 canvas-like approach to blending the results of multiple OpenGL drawings.
 * Supports all canvas modes, except "darken" for now.
 * Modelled after the HTML Canvas API as described in https://www.w3.org/TR/2dcontext/#compositing
 */
struct BlendMode {
    enum Mode : unsigned char {
        SOURCE_OVER,
        SOURCE_IN,
        SOURCE_OUT,
        SOURCE_ATOP,
        DESTINATION_OVER,
        DESTINATION_IN,
        DESTINATION_OUT,
        DESTINATION_ATOP,
        LIGHTER,
        COPY,
        XOR,
    };

    /** Blend mode for colors. */
    Mode rgb;

    /** Blend mode for transparency. */
    Mode alpha;

    /** Default Constructor. */
    BlendMode();

    /** Single blend mode for both rgb and alpha. */
    BlendMode(const Mode mode);

    /** Separate blend modes for both rgb and alpha. */
    BlendMode(const Mode color, const Mode alpha);

    /** Applies the blend mode to OpenGL.
     * A valid OpenGL context must exist before calling this function.
     */
    void apply() const;
};

} // namespace notf
