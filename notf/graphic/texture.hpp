#pragma once

#include "notf/meta/smart_ptr.hpp"

#include "notf/common/geo/size2.hpp"

#include "notf/graphic/graphics_context.hpp"

// TODO: [engine] a texture streaming method using buffers

NOTF_OPEN_NAMESPACE

// texture ========================================================================================================== //

/// A texture is an OpenGL Object that contains one or more images that all have the same image format.
/// A texture can be used in two ways: it can be the source of a texture access from a Shader, or it can be used as a
/// render target.
class Texture : public std::enable_shared_from_this<Texture> {

    friend Accessor<Texture, detail::GraphicsSystem>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Texture);

    /// Texture format.
    enum class Format {
        GRAYSCALE = 1, ///< one channel per pixel (grayscale)
        RGB = 3,       ///< 3 channels per pixel (color)
        RGBA = 4,      ///< 4 channels per pixel (color + alpha)
    };

    /// Filter used when sampling the texture and any of its mipmaps.
    enum class MinFilter {
        NEAREST,                ///< Nearest (in Manhattan distance) value to the center of the pixel
        LINEAR,                 ///< Weighted average of the four texels closest to the center of the pixel
        NEAREST_MIPMAP_NEAREST, ///< Gets the nearest texel from the closest mipmap
        NEAREST_MIPMAP_LINEAR,  ///< Gets the linearly interpolated texel from the closest mipmap
        LINEAR_MIPMAP_NEAREST,  ///< Weighted blend of the nearest texels of the two closest mipmaps
        LINEAR_MIPMAP_LINEAR,   ///< Weighted blend of the linearly interpolated texels of the two closest mipmaps
    };

    /// Filter used when only sampling the highest texture level.
    enum class MagFilter {
        NEAREST, ///< Nearest (in Manhattan distance) value to the center of the pixel
        LINEAR,  ///< Weighted average of the four texels closest to the center of the pixel
    };

    /// How a coordinate (c) outside the texture size (n) in a given direcion is handled.
    enum class Wrap {
        REPEAT,          ///< Only uses the fractional part of c, creating a repeating pattern (default)
        CLAMP_TO_EDGE,   ///< Clamps c to [1/2n, 1 - 1/2n]
        MIRRORED_REPEAT, ///< Like REPEAT when the integer part of c is even, 1 - frac(c) when c is odd
    };

    /// Codec used to store the texture in OpenGL.
    enum class Codec {
        RAW,  ///< All image formats that are decoded into raw pixels before upload (png, jpg, almost all of them...)
        ASTC, ///< ASTC compression
    };

    /// Type of the data passed into the texture.
    enum class DataType {
        BYTE,
        UBYTE,
        SHORT,
        USHORT,
        INT,
        UINT,
        HALF,
        FLOAT,
        USHORT_5_6_5,
    };

    /// Arguments used to initialize a Texture.
    struct Args {
        /// Filter used when sampling the texture and any of its mipmaps.
        MinFilter min_filter = MinFilter::LINEAR_MIPMAP_LINEAR;

        /// Filter used when only sampling the highest texture level.
        MagFilter mag_filter = MagFilter::LINEAR;

        /// Horizontal texture wrap.
        Wrap wrap_horizontal = Wrap::REPEAT;

        /// Vertical texture wrap.
        Wrap wrap_vertical = Wrap::REPEAT;

        /// Automatically generate mipmaps for textures load from a file.
        bool create_mipmaps = true;

        /// Immutable textures provide faster lookup but cannot change their format or size (but content).
        bool make_immutable = true;

        /// Format of the created texture, is ignored when loading a texture from file.
        Format format = Format::RGB;

        /// Type of the data passed into the texture.
        /// Also used to define the type of data written into a texture attached to a FrameBuffer.
        DataType data_type = DataType::UBYTE;

        /// Codec used to store the texture in OpenGL.
        Codec codec = Codec::RAW;

        /// Use a linear (RGB) or non-linear (SRGB) color-space.
        /// Usually textures are stored non-linearly, while render targets use a linear color-space.
        bool is_linear = true;

        /// Anisotropy factor - is only used if the anisotropic filtering extension is supported.
        /// A value <= 1 means no anisotropic filtering.
        float anisotropy = 1;
    };

    // methods --------------------------------------------------------------------------------- //
private:
    NOTF_CREATE_SMART_FACTORIES(Texture);

    /// Value Constructor.
    /// @param id       OpenGL texture ID.
    /// @param target   How the texture is going to be used by OpenGL.
    ///                 (see https://www.khronos.org/registry/OpenGL-Refpages/es3/html/glTexImage2D.xhtml)
    /// @param name     Application-unique name of the Texture.
    /// @param size     Size of the Texture in pixels.
    /// @param format   Texture format.
    Texture(const GLuint id, const GLenum target, std::string name, Size2i size, const Format format);

public:
    NOTF_NO_COPY_OR_ASSIGN(Texture);

    /// Creates an valid but transparent texture in memory.
    /// @param name         Application-unique name of the Texture.
    /// @param size         Size of the texture in pixels.
    /// @param args         Texture arguments.
    /// @throws ValueError  If the texture would have an invalid size.
    static TexturePtr create_empty(std::string name, Size2i size, const Args& args = s_default_args);

    /// Loads a texture from a given file.
    /// @param file_path        Path to a texture file.
    /// @param name             Application-unique name of the Texture.
    /// @param args             Arguments to initialize the texture.
    /// @throws ResourceError   If the file could not be loaded.
    /// @return Texture instance, is empty if the texture could not be loaded.
    static TexturePtr load_image(const std::string& file_path, std::string name, const Args& args = s_default_args);

    /// Destructor.
    ~Texture();

    /// The OpenGL ID of this Texture.
    TextureId get_id() const noexcept { return m_id; }

    /// Checks if the Texture is still valid.
    /// A Texture should always be valid - the only way to get an invalid one is to remove the GraphicsSystem while
    /// still holding on to shared pointers of a Texture that lived in the removed GraphicsSystem.
    bool is_valid() const noexcept { return m_id.is_valid(); }

    /// Texture target, e.g. GL_TEXTURE_2D for standard textures.
    GLenum get_target() const noexcept { return m_target; }

    /// The name of this Texture.
    const std::string& get_name() const noexcept { return m_name; }

    /// The size of this texture.
    const Size2i& get_size() const noexcept { return m_size; }

    /// The format of this Texture.
    const Format& get_format() const noexcept { return m_format; }

    /// Sets a new filter mode when the texture pixels are smaller than scren pixels.
    void set_min_filter(const MinFilter filter);

    /// Sets a new filter mode when the texture pixels are larger than scren pixels.
    void set_mag_filter(const MagFilter filter);

    /// Sets a new horizonal wrap mode.
    void set_wrap_x(const Wrap wrap);

    /// Sets a new vertical wrap mode.
    void set_wrap_y(const Wrap wrap);

    /// Completely fills the Texture with a given color.
    void flood(const Color& color);

private:
    /// Convenience method used to set all sorts of texture-related paramters.
    /// @param name     Parameter to set.
    /// @param value    New parameter value.
    void _set_parameter(const GLenum name, const GLint value);

    /// Deallocates the Texture data and invalidates the Texture.
    void _deallocate();

    // fields ---------------------------------------------------------------------------------- //
private:
    /// OpenGL ID of this Shader.
    TextureId m_id = 0;

    /// Texture target, e.g. GL_TEXTURE_2D for standard textures.
    GLenum m_target;

    /// The name of this Texture.
    const std::string m_name;

    /// The size of this texture.
    const Size2i m_size;

    /// Texture format.
    const Format m_format;

    /// Default arguments.
    static const Args s_default_args;
};

// accessors -------------------------------------------------------------------------------------------------------- //

template<>
class Accessor<Texture, detail::GraphicsSystem> {
    friend detail::GraphicsSystem;

    /// Deallocates the Texture data and invalidates the Texture.
    static void deallocate(Texture& texture) { texture._deallocate(); }
};

NOTF_CLOSE_NAMESPACE
