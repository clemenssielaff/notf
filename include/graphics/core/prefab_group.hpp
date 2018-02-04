#pragma once

#include <memory>
#include <sstream>

#include "./index_array.hpp"
#include "./prefab.hpp"
#include "./vertex_array.hpp"
#include "common/exception.hpp"
#include "common/forwards.hpp"
#include "common/matrix4.hpp"

namespace notf {

// ===================================================================================================================//

/// A prefab group contains 0-n prefabs that share the same vertex layout, and are rendered with the same shader.
/// It contains a single vertex buffer containing the vertices of all prefab types and a single index array to store
/// indices into the vertex buffer.
/// The group also contains an instance buffer that is repeatedly filled by each prefab type to render its instances.
template<typename VERTEX_ARRAY, typename INSTANCE_ARRAY, ENABLE_IF_SUBCLASS(VERTEX_ARRAY, VertexArrayType),
         ENABLE_IF_SUBCLASS(INSTANCE_ARRAY, VertexArrayType)>
class PrefabGroup {

    template<typename>
    friend class PrefabFactory;

    // types ---------------------------------------------------------------------------------------------------------//
public:
    using vertex_array_t = VERTEX_ARRAY;

    using index_array_t = IndexArray<GLuint>;

    using instance_array_t = INSTANCE_ARRAY;

    using InstanceData = typename instance_array_t::Vertex;

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(PrefabGroup)

    /// Default constructor.
    PrefabGroup()
        : m_vao_id(0)
        , m_vertex_array(std::make_unique<vertex_array_t>())
        , m_index_array(std::make_unique<index_array_t>())
        , m_instance_array()
    {
        // create the per-instance array
        VertexArrayType::Args instance_args;
        instance_args.per_instance = true;
        instance_args.usage        = GL_DYNAMIC_DRAW;
        m_instance_array           = std::make_unique<instance_array_t>(std::move(instance_args));
    }

    /// Destructor.
    ~PrefabGroup() { gl_check(glDeleteVertexArrays(1, &m_vao_id)); }

    /// Initializes the library.
    /// Call this method once, after all prefabs have been added using PrefabFactories.
    /// @throws std::runtime_error   If the PrefabGroup has already been initialized once.
    /// @throws std::runtime_error   If the OpenGL VAO could not be generated.
    void init()
    {
        if (m_vao_id) {
            throw_runtime_error("Cannot re-initialize a previously initialized PrefabGroup.");
        }

        gl_check(glGenVertexArrays(1, &m_vao_id));
        if (!m_vao_id) {
            throw_runtime_error("Failed to allocate the PrefabLibary VAO");
        }

        gl_check(glBindVertexArray(m_vao_id));
        static_cast<vertex_array_t*>(m_vertex_array.get())->init();
        static_cast<index_array_t*>(m_index_array.get())->init();
        static_cast<instance_array_t*>(m_instance_array.get())->init();
        gl_check(glBindVertexArray(0));
    }

    /// Returns a prefab type by its name.
    /// @throws std::runtime_error   If the name is unknown.
    std::shared_ptr<PrefabType<InstanceData>> prefab_type(std::string& name)
    {
        for (auto& type : m_prefab_types) {
            if (type->name() == name) {
                return type;
            }
        }
        std::stringstream ss;
        ss << "Unkown prefab type \"" << name << "\"";
        throw_runtime_error(ss.str());
    }

    /// Go through all prefab types of this group and render all instances of each type.
    void render()
    {
        // TODO: [engine] No front-to-back sorting of prefabs globally or even just within its group

        gl_check(glBindVertexArray(m_vao_id));
        for (const auto& prefab_type : m_prefab_types) {

            // skip prefabs with no instances
            std::vector<std::shared_ptr<PrefabInstance<InstanceData>>> instances = prefab_type->instances();
            if (instances.empty()) {
                continue;
            }

            { // update the instance buffer
                std::vector<InstanceData> instance_data;
                instance_data.reserve(instances.size());
                for (const std::shared_ptr<PrefabInstance<InstanceData>>& instance : instances) {
                    instance_data.push_back(instance->data());
                }

                instance_array_t* instance_array = static_cast<instance_array_t*>(m_instance_array.get());
                instance_array->buffer() = std::move(instance_data);
                instance_array->init();
            }

            // render all instances
            gl_check(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, prefab_type->size(), m_index_array->type(), 0,
                                                       instances.size(), prefab_type->offset()));
        }
        gl_check(glBindVertexArray(0));
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /** OpenGL handle of the internal vertex array object. */
    GLuint m_vao_id;

    /** Attributes for the prefabs' vertices. */
    std::unique_ptr<VertexArrayType> m_vertex_array;

    /** Vertex indices used to draw the prefabs. */
    std::unique_ptr<IndexArrayType> m_index_array;

    /** Per-instance attributes - updated before each instanced render call. */
    std::unique_ptr<VertexArrayType> m_instance_array;

    /** All prefab types contained in this library. */
    std::vector<std::shared_ptr<PrefabType<InstanceData>>> m_prefab_types;
};

} // namespace notf
