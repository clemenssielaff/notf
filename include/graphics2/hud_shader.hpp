#pragma once

#include <string>

#include "graphics/gl_forwards.hpp"
#include "graphics2/shader.hpp"

namespace notf {

class RenderBackend;

// TODO: I don't know if the separation between HUD_Layer and HUD_Shader really makes sense
// in Nanovg everything is put together into a "Context"
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

    /** The OpenGL ID of the Shader program. */
    GLuint get_id() const { return m_shader.get_id(); }

private: // methods
    static Sources _create_source(const RenderBackend& render_backend);

public: // fields
    Sources m_sources;

    Shader m_shader;

    GLint m_loc_viewsize;
    GLint m_loc_texture;

    GLuint m_loc_buffer;

    GLuint m_fragment_buffer;
    GLuint m_vertex_array;

    GLuint m_vertex_buffer;

//    int m_frag_size; // don't need this because I only support FragmentUniform
};

} // namespace notf
