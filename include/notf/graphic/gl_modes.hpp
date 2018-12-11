#pragma once

#include "notf/graphic/fwd.hpp"

NOTF_OPEN_NAMESPACE

// blend mode ======================================================================================================= //

/// HTML5 canvas-like approach to blending the results of multiple OpenGL drawings.
/// Modelled after the HTML Canvas API as described in https://www.w3.org/TR/2dcontext/#compositing
/// The source image is the image being rendered, and the destination image the current state of the bitmap.
struct BlendMode {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Blend mode, can be set for RGB and the alpha channel separately.
    enum Mode : unsigned char {
        SOURCE_OVER,      ///< Display the source image wherever the source image is opaque, the destination image
                          /// elsewhere (default).
        SOURCE_IN,        ///< Display the source image where the both are opaque, transparency elsewhere.
        SOURCE_OUT,       ///< Display the source image where the the source is opaque and the destination transparent,
                          ///  transparency elsewhere.
        SOURCE_ATOP,      ///< Source image wherever both images are opaque.
                          ///  Display the destination image wherever the destination image is opaque but the source
                          ///  image is transparent. Display transparency elsewhere.
        DESTINATION_OVER, ///< Same as SOURCE_OVER with the destination instead of the source.
        DESTINATION_IN,   ///< Same as SOURCE_IN with the destination instead of the source.
        DESTINATION_OUT,  ///< Same as SOURCE_OUT with the destination instead of the source.
        DESTINATION_ATOP, ///< Same as SOURCE_ATOP with the destination instead of the source.
        LIGHTER,          ///< The sum of the source image and destination image, with 255 (100%) as a limit.
        COPY,             ///< Source image instead of the destination image (overwrite destination).
        XOR,              ///< Exclusive OR of the source image and destination image.
        DEFAULT = SOURCE_OVER,
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    BlendMode() : rgb(DEFAULT), alpha(DEFAULT) {}

    /// Value constructor.
    /// @param mode Single blend mode for both rgb and alpha.
    BlendMode(const Mode mode) : rgb(mode), alpha(mode) {}

    /// Separate blend modes for both rgb and alpha.
    /// @param color    RGB blend mode.
    /// @param alpha    Alpha blend mode.
    BlendMode(const Mode color, const Mode alpha) : rgb(color), alpha(alpha) {}

    /// Equality operator.
    /// @param other    Blend mode to test against.
    bool operator==(const BlendMode& other) const { return rgb == other.rgb && alpha == other.alpha; }

    /// Inequality operator.
    /// @param other    Blend mode to test against.
    bool operator!=(const BlendMode& other) const { return rgb != other.rgb || alpha != other.alpha; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Blend mode for colors.
    Mode rgb;

    /// Blend mode for transparency.
    Mode alpha;
};

// cull face ======================================================================================================== //

/// Direction to cull in the culling test.
enum class CullFace : unsigned char {
    BACK,  ///< Do not render back-facing faces (default).
    FRONT, ///< Do not render front-facing faces.
    BOTH,  ///< Cull all faces.
    NONE,  ///< Render bot front- and back-facing faces.
    DEFAULT = BACK,
};

NOTF_CLOSE_NAMESPACE
