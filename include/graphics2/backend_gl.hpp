#pragma once

#include "graphics/gl_forwards.hpp"

namespace notf {

struct Shader2D {
    GLuint program;
    GLuint fragment_shader;
    GLuint vertex_shader;

    GLint viewsize_location;
    GLint texture_location;
    GLint buffer_location; // uniform buffer, called "frag" in nanovg
};

class OpenGLRenderBackend {

public: // methods
    OpenGLRenderBackend(bool fake_2daa = true)
        : m_fake_2daa(fake_2daa)
        , m_2d_shader(_produce_shader(*this))
    {
    }

private: // static methods

    /** Produces a customized OpenGL shader for the given backend.*/
    static Shader2D _produce_shader(const OpenGLRenderBackend& backend);

private: // fields
    /** Flag indicating whether or not 2D shapes should simulate their own antialiasing or not.
     * In a purely 2D application, this flag should be set to true since simulated antialiasing is cheaper than
     * full blown multisampling.
     * However, in a 3D application, you will most likely require true multisampling anyway, in which case we don't
     * need the fake antialiasing on top.
     */
    const bool m_fake_2daa;

    /** Shader used to */
    Shader2D m_2d_shader;

    /* Cached state to avoid unnecessary rebindings. */
    GLuint m_bound_texture;
    GLuint m_stencil_mask;
    GLenum m_stencil_func;
    GLint  m_stencil_func_ref;
    GLuint m_stencil_func_mask;
};

} // namespace notf
