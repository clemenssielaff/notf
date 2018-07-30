#pragma once

#include "common/matrix4.hpp"
#include "graphics/index_array.hpp"
#include "graphics/prefab.hpp"
#include "graphics/vertex_array.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// A prefab group contains 0-n prefabs that share the same vertex layout, and are rendered with the same shader.
/// It contains a single vertex buffer containing the vertices of all prefab types and a single index array to store
/// indices into the vertex buffer.
/// The group also contains an instance buffer that is repeatedly filled by each prefab type to render its instances.
template<typename VERTEX_ARRAY, typename INSTANCE_ARRAY,
         typename = std::enable_if_t<std::is_base_of<VertexArrayType, VERTEX_ARRAY>::value>,
         typename = std::enable_if_t<std::is_base_of<VertexArrayType, INSTANCE_ARRAY>::value>>
class PrefabGroup {

    template<typename>
    friend class PrefabFactory;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    using vertex_array_t = VERTEX_ARRAY;

    using index_array_t = IndexArray<GLuint>;

    using instance_array_t = INSTANCE_ARRAY;

    using InstanceData = typename instance_array_t::Vertex;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(PrefabGroup);

    /// Default constructor.
    PrefabGroup() : m_vertex_array(std::make_unique<vertex_array_t>()), m_index_array(std::make_unique<index_array_t>())
    {
        // create the per-instance array
        VertexArrayType::Args instance_args;
        instance_args.per_instance = true;
        instance_args.usage = GLUsage::DYNAMIC_DRAW;
        m_instance_array = std::make_unique<instance_array_t>(std::move(instance_args));
    }

    /// Destructor.
    ~PrefabGroup() { notf_check_gl(glDeleteVertexArrays(1, &m_vao_id)); }

    /// Initializes the library.
    /// Call this method once, after all prefabs have been added using PrefabFactories.
    /// @throws runtime_error   If the PrefabGroup has already been initialized once.
    /// @throws runtime_error   If the OpenGL VAO could not be generated.
    void init()
    {
        if (m_vao_id) {
            NOTF_THROW(runtime_error, "Cannot re-initialize a previously initialized PrefabGroup.");
        }

        notf_check_gl(glGenVertexArrays(1, &m_vao_id));
        if (!m_vao_id) {
            NOTF_THROW(runtime_error, "Failed to allocate the PrefabLibary VAO");
        }

        {
            VaoBindGuard _(m_vao_id);
            static_cast<vertex_array_t*>(m_vertex_array.get())->init();
            static_cast<index_array_t*>(m_index_array.get())->init();
            static_cast<instance_array_t*>(m_instance_array.get())->init();
        }
    }

    /// Checks if a prefab with the given name exist.
    /// @param name Prefab name to look for.
    bool has_prefab_type(const std::string& name) const
    {
        const auto it = std::find_if(m_prefab_types.begin(), m_prefab_types.end(),
                                     [&](const auto& prefab_type) -> bool { return prefab_type->name() == name; });
        return (it != m_prefab_types.end());
    }

    /// Returns a prefab type by its name.
    /// @throws runtime_error   If the name is unknown.
    std::shared_ptr<PrefabType<InstanceData>> prefab_type(const std::string& name) const
    {
        const auto it = std::find_if(m_prefab_types.begin(), m_prefab_types.end(),
                                     [&](const auto& prefab_type) -> bool { return prefab_type->name() == name; });
        if (it != m_prefab_types.end()) {
            return *it;
        }
        NOTF_THROW(runtime_error, "Unkown prefab type \"{}\"", name);
    }

    /// Go through all prefab types of this group and render all instances of each type.
    void render()
    {
        // TODO: [engine] No front-to-back sorting of prefabs globally or even just within its group
        VaoBindGuard _(m_vao_id);
        for (const auto& prefab_type : m_prefab_types) {

            // skip prefabs with no instances
            std::vector<std::shared_ptr<PrefabInstance<InstanceData>>> instances = prefab_type->get_instances();
            if (instances.empty()) {
                continue;
            }

            { // update the instance buffer
                std::vector<InstanceData> instance_data;
                instance_data.reserve(instances.size());
                for (const std::shared_ptr<PrefabInstance<InstanceData>>& instance : instances) {
                    instance_data.push_back(instance->get_data());
                }

                instance_array_t* instance_array = static_cast<instance_array_t*>(m_instance_array.get());
                instance_array->buffer() = std::move(instance_data);
                instance_array->init();
            }

            // render all instances
            notf_check_gl(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, prefab_type->get_size(),
                                                            m_index_array->get_type(), 0, instances.size(),
                                                            prefab_type->get_offset()));
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// OpenGL handle of the internal vertex array object.
    GLuint m_vao_id = 0;

    /// Attributes for the prefabs' vertices.
    std::unique_ptr<VertexArrayType> m_vertex_array;

    /// Vertex indices used to draw the prefabs.
    std::unique_ptr<IndexArrayType> m_index_array;

    /// Per-instance attributes - updated before each instanced render call.
    std::unique_ptr<VertexArrayType> m_instance_array;

    /// All prefab types contained in this library.
    std::vector<std::shared_ptr<PrefabType<InstanceData>>> m_prefab_types;
};

NOTF_CLOSE_NAMESPACE
