#pragma once

#include <memory>
#include <string>

#include "common/size2i.hpp"

struct NVGcontext;

namespace notf {

/** Manages the loading and setup of an OpenGL texture used in NanoVG. */
class Texture2 {

public: // enum
    /** Flags passed to Texture2::load(). */
    enum Flags {
        GENERATE_MIPMAPS = 1 << 0, // Generate mipmaps during creation of the image.
        REPEATX = 1 << 1, // Repeat image in X direction.
        REPEATY = 1 << 2, // Repeat image in Y direction.
        FLIPY = 1 << 3, // Flips (inverses) image in Y direction when rendered.
        PREMULTIPLIED = 1 << 4, // Image data has premultiplied alpha.
    };

public: // static methods
    /** Loads a texture from a given file.
    * @param nvg_context        NanoVG context of which this Texture is a part.
    * @param texture_path       Path to a texture file.
    * @param flags              Combination of `Texture::Flags`.
    * @throw std::runtime_error If the texture fails to load.
    */
    static std::shared_ptr<Texture2> load(NVGcontext* nvg_context, const std::string& texture_path, int flags);

public: // methods
    /// no copy / assignment
    Texture2(const Texture2&) = delete;
    Texture2& operator=(const Texture2&) = delete;

    ~Texture2();

    /** The OpenGL ID of this Texture2. */
    int get_id() const { return m_id; }

    /** Size of the Texture2 in pixels. */
    Size2i get_size() const;

protected: // methods
    /**
     * @param id            OpenGL texture ID.
     * @param nvg_context   NanoVG context of which this Texture is a part.
     */
    Texture2(const int id, NVGcontext* nvg_context)
        : m_id(id)
        , m_context(nvg_context)
    {
    }

private: // fields
    /** OpenGL ID of this Shader. */
    int m_id;

    /** NanoVG context of which this Texture is a part. */
    NVGcontext* m_context;
};

} // namespace notf
