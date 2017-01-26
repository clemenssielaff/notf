#pragma once

#include <string>

#include "graphics/gl_forwards.hpp"
#include "graphics2/shader.hpp"

namespace notf {

class RenderBackend;

class HUDShader {

private: // struct
    struct Sources {
        std::string vertex;
        std::string fragment;
    };

public: // methods
    /** Default Constructor - produces an invalid Shader. */
    HUDShader() = default;

    /** Constructor. */
    HUDShader(const RenderBackend& render_backend);

    /** Move Constructor. */
    HUDShader(HUDShader&& other);

    HUDShader& operator=(HUDShader&& other);

    /** Destructor. */
    ~HUDShader();

    /** Checks if the Shader is valid. */
    bool is_valid() const { return m_shader.is_valid(); }

private: // methods
    static Sources _create_source(const RenderBackend& render_backend);

private: // fields
    Sources m_sources;

    Shader m_shader;

    GLint m_loc_viewsize;
    GLint m_loc_texture;
#ifdef NOTF_OPENGL3
    GLuint m_loc_buffer;
#else
    GLint m_loc_buffer;
#endif

#ifdef NOTF_OPENGL3
    GLuint m_fragment_buffer;
    GLuint m_vertex_array;
#endif

    GLuint m_vertex_buffer;

    int m_frag_size;
};

} // namespace notf
