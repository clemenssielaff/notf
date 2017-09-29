#pragma once

#include <memory>
#include <sstream>

#include "common/exception.hpp"
#include "graphics/index_array.hpp"
#include "graphics/prefab.hpp"
#include "graphics/vertex_array.hpp"

#include "common/xform3.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(class, Shader)

//*********************************************************************************************************************/

/**
 *
 */
template <typename VERTEX_ARRAY, typename INSTANCE_ARRAY,
          ENABLE_IF_SUBCLASS(VERTEX_ARRAY, VertexArrayType), ENABLE_IF_SUBCLASS(INSTANCE_ARRAY, VertexArrayType)>
class PrefabLibrary {

    template <typename>
    friend class PrefabFactory;

public: // types ******************************************************************************************************/
    using vertex_array_t = VERTEX_ARRAY;

    using instance_array_t = INSTANCE_ARRAY;

    using InstanceData = typename instance_array_t::Vertex;

public: // methods ****************************************************************************************************/
    DISALLOW_COPY_AND_ASSIGN(PrefabLibrary)

    /** Constructor. */
    PrefabLibrary(ShaderPtr shader)
        : m_vao_id(0)
        , m_shader(std::move(shader))
        , m_vertex_array(std::make_unique<VERTEX_ARRAY>())
        , m_index_array(std::make_unique<IndexArray<GLuint>>())
        , m_instance_array()
    {
        // create the per-instance array
        VertexArrayType::Args instance_args;
        instance_args.per_instance = true;
        instance_args.usage        = GL_DYNAMIC_DRAW;
        m_instance_array           = std::make_unique<INSTANCE_ARRAY>(std::move(instance_args));
    }

    /** Destructor. */
    ~PrefabLibrary()
    {
        glDeleteVertexArrays(1, &m_vao_id);
    }

    /** Initializes the library.
     * Call this method once, after all prefabs have been added using PrefabFactories.
     * @throws std::runtime_error   If the PrefabLibrary has already been initialized once.
     * @throws std::runtime_error   If the OpenGL VAO could not be generated.
     */
    void init()
    {
        if (m_vao_id) {
            throw_runtime_error("Cannot re-initialize a previously initialized PrefabLibrary.");
        }

        glGenVertexArrays(1, &m_vao_id);
        if (!m_vao_id) {
            throw_runtime_error("Failed to allocate the PrefabLibary VAO");
        }

        ///////////
        {
            const Xform3f move      = Xform3f::translation(0, 500, -1000);
            std::array<float, 16> offset;
            for (size_t i = 0; i < 16; ++i) {
                offset[i] = move.as_ptr()[i];
            }
            static_cast<INSTANCE_ARRAY*>(m_instance_array.get())->m_vertices.push_back(std::make_tuple(offset));
        }
        {
            const Xform3f move      = Xform3f::translation(0, -500, -1000);
            std::array<float, 16> offset;
            for (size_t i = 0; i < 16; ++i) {
                offset[i] = move.as_ptr()[i];
            }
            static_cast<INSTANCE_ARRAY*>(m_instance_array.get())->m_vertices.push_back(std::make_tuple(offset));
        }

        ///////////

        glBindVertexArray(m_vao_id);
        m_vertex_array->init(m_shader);
        m_index_array->init();
        m_instance_array->init(m_shader);
        glBindVertexArray(0); // TODO: store VAO in the GraphicsContext as a stack (like Shaders)
    }

    /** Returns a prefab type by its name.
     * @throws std::runtime_error   If the name is unknown.
     */
    std::shared_ptr<Prefab<InstanceData>> prefab_type(std::string& name)
    {
        for (auto& type : m_prefabs) {
            if (type.first == name) {
                return type.second;
            }
        }
        std::stringstream ss;
        ss << "Unkown prefab type \"" << name << "\"";
        throw std::runtime_error(ss.str());
    }

    void render()
    {
        glBindVertexArray(m_vao_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_array->id());
        //glDrawElements(GL_TRIANGLES, m_index_array->size(), m_index_array->type(), 0);
        glDrawElementsInstanced(GL_TRIANGLES, m_index_array->size(), m_index_array->type(), 0, 2);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        check_gl_error();
    }

private: // fields ****************************************************************************************************/
public:  // TODO: TEMP
    /** OpenGL handle of the internal vertex array object. */
    GLuint m_vao_id;

    /** Shader used to draw prefabs from this library. */
    ShaderPtr m_shader;

    /** Attributes for the prefabs' vertices. */
    std::unique_ptr<VertexArrayType> m_vertex_array;

    /** Vertex indices used to draw the prefabs. */
    std::unique_ptr<IndexArrayType> m_index_array;

    /** Per-instance attributes - updated before each instanced render call. */
    std::unique_ptr<VertexArrayType> m_instance_array;

    /** All prefab types contained in this library. */
    std::vector<std::pair<std::string, std::shared_ptr<Prefab<InstanceData>>>> m_prefabs;
};

} // namespace notf
