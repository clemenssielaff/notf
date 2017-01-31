#pragma once

namespace notf {

/* Short design discussion.
 * The graphics module in NoTF consists of a stack of abstractions.
 * They are (in reverse order, meaning the first is the lowest):
 *
 *     1. OpenGL
 *        In all its glory.
 *
 *     2. RenderBackend
 *        Holds application constant state, like the version of OpenGL used or if multisampling was enabled.
 *        Doesn't do anything itself, except rendering RenderLayers.
 *
 *     3. RenderLayer
 *        One Layer per render setup, most UIs (2D, canvas-style drawings) can probably do with just one layer.
 *        The RenderLayer handles the drawing to OpenGL.
 *        Holds frame-specifc state, like window size.
 *
 *     4. Canvas (or Patch)
 *        An intermediate object owned by Widgets to store the widget state.
 *        Defines how to draw the Widget onto the layer.
 *        This way, we can define the canvas once (from Python) and render the Widget multiple times without the need
 *        to repeat the (potentially) expensive redefinition.
 *        Only when the Widget's contents have actually changed, must the canvas be updated.
 *
 *     5. Painter
 *        Is created by the Canvas, passed to the Widget's `paint` function and discarded after return.
 *        Holds only rudimentary state, whatever is needed to update the Canvas.
 *        This is the thing that the user interacts with when subclassing Widgets.
 */

/** The RenderBackend is constructed once per session and is used to configure the RenderLayers.
 * Unlike Nanovg, NoTF has no support for OpenGL2 but instead requires 3.3+ because by the time NoTF hits big,
 * OpenGl 3.3 will be over 8 years old and I just cannot be bothered.
 */
class RenderBackend {

public: // enum
    /** Type of this RenderBackend. */
    enum class Type : unsigned char {
        OPENGL_3,
        GLES_3,
    };

public: // methods
    /** Constructor. */
    RenderBackend(const bool msaa)
        : has_msaa(msaa)
    {
    }

public: // fields
    /** Flag indicating whether OpenGL renders with multi-sample antialiasing enabled or not.
     * This determines, whether the HUDShader will provide geometric antialiasing for its 2D shapes or not.
     * In a purely 2D application, this flag should be set to `false` since geometric antialiasing is cheaper than
     * full blown multisampling and looks just as good.
     * However, in a 3D application, you will most likely require true multisampling anyway, in which case we don't
     * need the redundant geometrical antialiasing on top.
     */
    const bool has_msaa;

public: // static fields
    /** Type of this RenderBackend. */
    static const Type type;
};

} // namespace notf
