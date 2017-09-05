#pragma once

#include <tuple>
#include <vector>

#include "core/opengl.hpp"
#include "graphics/gl_utils.hpp"
#include "graphics/shader.hpp"
#include "utils/static_string.hpp"

namespace notf {

struct VertexPos {
    constexpr static StaticString name = "vPos";
    constexpr static GLint size        = 3;
    constexpr static GLenum type       = GL_FLOAT;
};

struct VertexColor {
    constexpr static StaticString name = "vColor";
    constexpr static GLint size        = 3;
    constexpr static GLenum type       = GL_FLOAT;
};

template <typename... Ts>
class VertexBuffer {

public:
    using Vertex = std::tuple<Ts...>;

private:
    struct attribute { // see http://en.cppreference.com/w/cpp/utility/tuple/tuple_element
        template <std::size_t N>
        using type = typename std::tuple_element<N, std::tuple<Ts...>>::type;
    };

public:
    VertexBuffer(ShaderPtr shader, std::vector<Vertex>&& vertices)
        : m_buffer_id(0)
        , m_shader(std::move(shader))
        , m_vertices(std::move(vertices))
    {
        glGenBuffers(1, &m_buffer_id);
        if (m_buffer_id) {
            glBindBuffer(GL_ARRAY_BUFFER, m_buffer_id);
            glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], GL_STATIC_DRAW);

            _init_buffer<0, Ts...>();
        }
    }

    ~VertexBuffer()
    {
        glDeleteBuffers(1, &m_buffer_id);
    }

private: // methods ***************************************************************************************************/
    template <size_t INDEX, typename FIRST, typename SECOND, typename... REST>
    void _init_buffer()
    {
        _define_attribute<INDEX, FIRST>();
        _init_buffer<INDEX + 1, SECOND, REST...>();
    }

    template <size_t INDEX, typename LAST>
    void _init_buffer()
    {
        _define_attribute<LAST>();
    }

    template <size_t INDEX, typename ATTRIBUTE>
    void _define_attribute()
    {
        const GLuint attribute_id   = m_shader->attribute(std::string(ATTRIBUTE::name.ptr, ATTRIBUTE::name.size));
        static const GLsizei stride = sizeof(Vertex);
        glEnableVertexAttribArray(attribute_id);
        size_t offset = 0;
        for (size_t i = 0; i < INDEX; ++i) {
            offset += sizeof(attribute::template type<i>);
        }
        glVertexAttribPointer(
            attribute_id, ATTRIBUTE::size, ATTRIBUTE::type, /*normalized*/ GL_FALSE, stride, gl_buffer_offset(offset));
    }

private: // fields ****************************************************************************************************/
    GLuint m_buffer_id;

    ShaderPtr m_shader;

    std::vector<Vertex> m_vertices;
};

} // namespace notf
