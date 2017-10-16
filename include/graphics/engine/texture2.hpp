#pragma once

#include <memory>
#include <string>

#include "common/meta.hpp"
#include "graphics/engine/gl_forwards.hpp"

namespace notf {

struct Color;
class GraphicsContext;

DEFINE_SHARED_POINTERS(class, Texture2);

template <typename value_t, FWD_ENABLE_IF_ARITHMETIC(value_t)>
struct _Size2;
using Size2i = _Size2<int, true>;

//*********************************************************************************************************************/

/** Manages the loading and setup of an OpenGL texture.
 *
 * Texture and GraphicsContext
 * ===========================
 * A Texture needs a valid GraphicsContext (which in turn refers to an OpenGL context), since the Texture class itself
 * does not store any image data, only the OpenGL ID and metadata.
 * You create a Texture by calling `GraphicsContext::load_texture(texture_path)`, which attaches the GraphicsContext to
 * the Texture.
 * The return value is a shared pointer, which you own.
 * However, the GraphicsContext does keep a weak pointer to the Texture and will deallocate it when it's itself removed.
 * In this case, the remaining Texture will become invalid and you'll get a warning message.
 * In a well-behaved program, all Textures should have gone out of scope by the time the GraphicsContext is destroyed.
 * This behaviour is similar to the handling of Shaders.
 */
class Texture2 : public std::enable_shared_from_this<Texture2> {

    friend class GraphicsContext; // creates and finally invalidates all of its Textures when it is destroyed

public: // enums ******************************************************************************************************/
    /** Texture format. */
    enum class Format : unsigned char {
        GRAYSCALE = 1, //* one byte per pixel (grayscale)
        RGB       = 3, //* 3 bytes per pixel (color)
        RGBA      = 4, //* 4 bytes per pixel (color + alpha)
    };

    /** Filter used when sampling the texture and any of its mipmaps. */
    enum class MinFilter : unsigned char {
        NEAREST,                //* Nearest (in Manhattan distance) value to the center of the pixel
        LINEAR,                 //* Weighted average of the four texels closest to the center of the pixel
        NEAREST_MIPMAP_NEAREST, //* Gets the nearest (s.a.) texel from the closest mipmap
        NEAREST_MIPMAP_LINEAR,  //* Gets the linearly interpolated (s.a.) texel from the closest mipmap
        LINEAR_MIPMAP_NEAREST,  //* Weighted blend of the nearest (s.a.) texels of the two closest mipmaps
        LINEAR_MIPMAP_LINEAR,   //* Weighted blend of the linearly interpolated (s.a.) texels of the two closest mipmaps
    };

    /** Filter used when only sampling the highest texture level. */
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

    /** Codec used to store the texture in OpenGL. */
    enum class Codec : unsigned char {
        RAW,    //* All image formats that are decoded into raw pixels before upload (png, jpg, almost all of them...)
        ASTC,   //* ASTC compression
    };

    /** Arguments used to initialize a Texture. */
    struct Args {
        /** Filter used when sampling the texture and any of its mipmaps. */
        MinFilter min_filter = MinFilter::LINEAR_MIPMAP_LINEAR;

        /** Filter used when only sampling the highest texture level. */
        MagFilter mag_filter = MagFilter::LINEAR;

        /** Horizontal texture wrap. */
        Wrap wrap_horizontal = Wrap::REPEAT;

        /** Vertical texture wrap. */
        Wrap wrap_vertical = Wrap::REPEAT;

        /** Automatically generate mipmaps for textures load from a file. */
        bool generate_mipmaps = true;

        /** Immutable textures provide faster lookup but cannot change their format or size (but content). */
        bool make_immutable = true;

        /** Format of the created texture, is ignored when loading a texture from file. */
        Format format = Format::RGB;

        /** Codec used to store the texture in OpenGL. */
        Codec codec = Codec::RAW;
    };

public: // types ******************************************************************************************************/
    /** Texture scope RAII helper. */
    struct Scope {

        /** Constructor, binds the texture. */
        Scope(Texture2* texture);

        /** Move without re-binding. */
        Scope(Scope&& other)
            : m_texture(other.m_texture)
        {
            other.m_texture = nullptr;
        }

        /** Destructor, unbinds the texture again. */
        ~Scope();

        /** Bound texture. */
        Texture2* m_texture;
    };
    friend struct Scope;

public: // static methods *********************************************************************************************/
    /** Loads a texture from a given file.
     * @param context   Render Context in which the texture lives.
     * @param file_path Path to a texture file.
     * @param args      Arguments to initialize the texture.
     * @return          Texture2 instance, is empty if the texture could not be loaded.
     */
    static std::shared_ptr<Texture2> load_image(GraphicsContext& context, const std::string file_path,
                                                const Args& args = s_default_args);

    /** Creates an empty texture in memory.
     * @param context   Render Context in which the Texture lives.
     * @param name      Name of the Texture.
     * @param size      Size of the Texture.
     * @param format    Texture format.
     */
    static std::shared_ptr<Texture2> create_empty(GraphicsContext& context, const std::string name, const Size2i& size,
                                                  const Args& args = s_default_args);

private: // constructor ***********************************************************************************************/
    /** Factory. */
    static std::shared_ptr<Texture2> _create(const GLuint id, GraphicsContext& context, const std::string name,
                                             const GLint width, const GLint height, const Format format);

    /** Value Constructor.
     * @param id        OpenGL texture ID.
     * @param context   Render Context in which the Texture lives.
     * @param name      Human readable name of the Texture.
     * @param width     Width of the loaded image in pixels.
     * @param height    Height of the loaded image in pixels.
     * @param format    Texture format.
     */
    Texture2(const GLuint id, GraphicsContext& context, const std::string name,
             const GLint width, const GLint height, const Format format);

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(Texture2)

    /** Destructor. */
    ~Texture2();

    /** A scope object that pushes this texture onto the stack and pops it on destruction. */
    Scope scope() { return Scope(this); }

    /** The OpenGL ID of this Texture2. */
    GLuint id() const { return m_id; }

    /** Checks if the Texture is still valid.
     * A Texture should always be valid - the only way to get an invalid one is to remove the GraphicsContext while
     * still holding on to shared pointers of a Texture that lived in the removed GraphicsContext.
     */
    bool is_valid() const { return m_id != 0; }

    /** The name of this Texture. */
    const std::string& name() const { return m_name; }

    /** Width of the loaded image in pixels. */
    GLint width() const { return m_width; }

    /** Height of the loaded image in pixels. */
    GLint height() const { return m_height; }

    /** The format of this Texture. */
    Format format() const { return m_format; }

    /** Returns the filter mode when the texture pixels are smaller than scren pixels. */
    MinFilter filter_min() const { return m_min_filter; }

    /** Returns the filter mode when the texture pixels are larger than scren pixels. */
    MagFilter filter_mag() const { return m_mag_filter; }

    /** Returns the horizonal wrap mode. */
    Wrap wrap_x() const { return m_wrap_x; }

    /** Returns the vertical wrap mode. */
    Wrap wrap_y() const { return m_wrap_y; }

    /** Sets a new filter mode when the texture pixels are smaller than scren pixels. */
    void set_min_filter(const MinFilter filter);

    /** Sets a new filter mode when the texture pixels are larger than scren pixels. */
    void set_mag_filter(const MagFilter filter);

    /** Sets a new horizonal wrap mode. */
    void set_wrap_x(const Wrap wrap);

    /** Sets a new vertical wrap mode. */
    void set_wrap_y(const Wrap wrap);

    /** Fills the Texture with a given color. */
    void fill(const Color& color);

private: // methods ***************************************************************************************************/
    /** Deallocates the Texture data and invalidates the Texture. */
    void _deallocate();

private: // fields ***************************************************************************************************/
    /** OpenGL ID of this Shader. */
    GLuint m_id;

    /** Render Context in which the Texture lives. */
    GraphicsContext& m_graphics_context;

    /** The name of this Texture. */
    const std::string m_name;

    /** Width of the loaded image in pixels. */
    const GLint m_width;

    /** Height of the loaded image in pixels. */
    const GLint m_height;

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

    /** Default arguments. */
    static const Args s_default_args;
};

} // namespace notf
