#pragma once

#include <memory>

#include "common/meta.hpp"
#include "graphics/gl_forwards.hpp"

namespace notf {

class IndexBufferType;

DEFINE_SHARED_POINTERS(class, VertexBufferType)
DEFINE_SHARED_POINTERS(class, Shader)

//*********************************************************************************************************************/

/**
 *
 */
class VertexObject {

public: // type *******************************************************************************************************/
    enum class RenderMode : unsigned char {
        POINTS,
        LINE_STRIP,
        LINE_LOOP,
        LINES,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
        TRIANGLES,
    };

public: // methods **************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(VertexObject)

    /** Constructor.
     *
     */
    VertexObject(ShaderPtr shader, VertexBufferTypePtr vertices,
                 const RenderMode mode = RenderMode::TRIANGLES, std::unique_ptr<IndexBufferType> indices = {});

    /** Destructor. */
    ~VertexObject();

    /** Renders this object. */
    void render() const;

private: // methods ***************************************************************************************************/
    /** Updates the object's render mode. */
    void _set_render_mode(const RenderMode mode);

private: // fields ****************************************************************************************************/
    /** OpenGL handle of the vertex array object. */
    GLuint m_vao_id;

    /** Render mode, corresponds to RenderMode enum values. */
    GLenum m_mode;

    /** Shader used to draw this vertex object. */
    ShaderPtr m_shader;

    /** Vertex buffer to draw this object from. */
    VertexBufferTypePtr m_vertices;

    /** Index buffer, laeve empty to draw vertices in the order they appear in the VertexBuffer. */
    std::unique_ptr<IndexBufferType> m_indices;
};

} // namespace notf
