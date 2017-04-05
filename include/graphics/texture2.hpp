#pragma once

#include <memory>
#include <string>

#include "graphics/gl_forwards.hpp"

namespace notf {

class RenderContext;

//*********************************************************************************************************************/

/** Manages the loading and setup of an OpenGL texture.
 *
 * Texture and RenderContext
 * =========================
 * A Texture needs a valid RenderContext (which in turn refers to an OpenGL context), since the Texture class itself
 * does not store any image data, only the OpenGL ID and metadata.
 * You create a Texture by calling `RenderContext::load_texture(texture_path)`, which attaches the RenderContext to the
 * Texture.
 * The return value is a shared pointer, which you own.
 * However, the RenderContext does keep a weak pointer to the Texture and will deallocate it when it is itself removed.
 * In this case, the remaining Texture will become invalid and you'll get a warning message.
 * In a well-behaved program, all Textures should have gone out of scope by the time the RenderContext is destroyed.
 * This behaviour is similar to the handling of Shaders.
 */
class Texture2 {

    friend class RenderContext; // creates and finally invalidates all of its Textures when it is destroyed

public: // enums
    enum class Format : unsigned char {
        GRAYSCALE, //* one byte per pixel (grayscale)
        RGB,       //* 3 bytes per pixel (color)
        RGBA,      //* 4 bytes per pixel (color + alpha)
    };

    enum class MinFilter : unsigned char {
        NEAREST,                //* Nearest (in Manhattan distance) value to the center of the pixel
        LINEAR,                 //* Weighted average of the four texels closest to the center of the pixel
        NEAREST_MIPMAP_NEAREST, //* Gets the nearest (s.a.) texel from the closest mipmap
        NEAREST_MIPMAP_LINEAR,  //* Gets the linearly interpolated (s.a.) texel from the closest mipmap
        LINEAR_MIPMAP_NEAREST,  //* Weighted blend of the nearest (s.a.) texels of the two closest mipmaps
        LINEAR_MIPMAP_LINEAR,   //* Weighted blend of the linearly interpolated (s.a.) texels of the two closest mipmaps
    };

    enum class MagFilter : unsigned char {
        NEAREST, //* Nearest (in Manhattan distance) value to the center of the pixel
        LINEAR,  //* Weighted average of the four texels closest to the center of the pixel
    };

    /** How a coordinate (c) outside the texture size (n) in a given direcion is handled. */
    enum class Wrap : unsigned char {
        REPEAT,          //* Only uses the fractional part of c, creating a repeating pattern (default)
        CLAMP_TO_EDGE,   //* Clamps c to [1/2n,  1 - 1/2n]
        MIRRORED_REPEAT, //* Like REPEAT when the integer part of c is even, 1 - frac(c) when c is odd
    };

public: // static methods
    /** Unbinds any current active Texture. */
    static void unbind();

private: // factory
    struct make_shared_enabler;

    /** Loads a texture from a given file.
     * @param context   Render Context in which the Texture lives.
     * @param file_path Path to a texture file.
     * @return          Texture2 instance, is empty if the texture could not be loaded.
     */
    static std::shared_ptr<Texture2> load(RenderContext* context, const std::string file_path);

    /** Value Constructor.
     * @param id        OpenGL texture ID.
     * @param context   Render Context in which the Texture lives.
     * @param name      Human readable name of the Texture.
     * @param width     Width of the loaded image in pixels.
     * @param height    Height of the loaded image in pixels.
     * @param format    Texture format.
     */
    Texture2(const GLuint id, RenderContext* context, const std::string name,
             const GLuint width, const GLuint height, const Format format);

public: // methods
    /// no copy / assignment
    Texture2(const Texture2&) = delete;
    Texture2& operator=(const Texture2&) = delete;

    /** Destructor. */
    ~Texture2();

    /** The OpenGL ID of this Texture2. */
    GLuint get_id() const { return m_id; }

    /** Checks if the Texture is still valid.
     * A Texture should always be valid - the only way to get an invalid one is to remove the RenderContext while still
     * holding on to shared pointers of a Texture that lived in the removed RenderContext.
     */
    bool is_valid() const { return m_id != 0; }

    /** The name of this Texture. */
    const std::string& get_name() const { return m_name; }

    /** Binds this Texture2 as the current active Texture.
     * @return  False if this Texture is invalid and cannot be bound.
     */
    bool bind() const;

    /** Width of the loaded image in pixels. */
    GLuint get_width() const { return m_width; }

    /** Height of the loaded image in pixels. */
    GLuint get_height() const { return m_height; }

    /** The format of this Texture. */
    Format get_format() const { return m_format; }

    /** Returns the filter mode when the texture pixels are smaller than scren pixels. */
    MinFilter get_filter_min() const { return m_min_filter; }

    /** Returns the filter mode when the texture pixels are larger than scren pixels. */
    MagFilter get_filter_mag() const { return m_mag_filter; }

    /** Returns the horizonal wrap mode. */
    Wrap get_wrap_x() const { return m_wrap_x; }

    /** Returns the vertical wrap mode. */
    Wrap get_wrap_y() const { return m_wrap_y; }

    /** Sets a new filter mode when the texture pixels are smaller than scren pixels. */
    void set_min_filter(const MinFilter filter);

    /** Sets a new filter mode when the texture pixels are larger than scren pixels. */
    void set_mag_filter(const MagFilter filter);

    /** Sets a new horizonal wrap mode. */
    void set_wrap_x(const Wrap wrap);

    /** Sets a new vertical wrap mode. */
    void set_wrap_y(const Wrap wrap);

private: // methods for RenderContext
    /** Deallocates the Texture data and invalidates the Texture. */
    void _deallocate();

private: // fields
    /** OpenGL ID of this Shader. */
    GLuint m_id;

    /** Render Context in which the Texture lives. */
    RenderContext* m_render_context;

    /** The name of this Texture. */
    const std::string m_name;

    /** Width of the loaded image in pixels. */
    const GLuint m_width;

    /** Height of the loaded image in pixels. */
    const GLuint m_height;

    /** Texture format. */
    const Format m_format;

    /** Filter mode when the texture pixels are smaller than scren pixels. */
    MinFilter m_min_filter;

    /** Filter mode when the texture pixels are larger than scren pixels. */
    MagFilter m_mag_filter;

    /** The horizontal wrap mode. */
    Wrap m_wrap_x;

    /** The vertical wrap mode. */
    Wrap m_wrap_y;
};

} // namespace notf
