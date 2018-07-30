#pragma once

#include <sstream>
#include <vector>

#include "common/color.hpp"
#include "common/exception.hpp"
#include "common/half.hpp"
#include "common/vector.hpp"
#include "common/vector2.hpp"
#include "common/vector3.hpp"
#include "common/vector4.hpp"
#include "graphics/prefab.hpp"
#include "graphics/prefab_group.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

namespace detail {

class PrefabFactoryImpl {

    // types -------------------------------------------------------------------------------------------------------- //
protected:
    /// Intermediate structure independent of the Vertex layout.
    struct Study {
        Vector3d position;
        Vector3d normal;
        Vector3d color;
        Vector2d tex_coord;
    };

    /// Result of a call to _produce().
    struct Product {
        std::vector<Study> studies;
        std::vector<GLuint> indices;
    };

    /// All Definition kinds.
    enum class Kind {
        BOX,
        SPHERE,
    };

    /// Base class of all Definitions so we can keep them all in a vector.
    struct Definition {
        Definition() = default;
        Definition(const Definition&) = default;
        virtual ~Definition();
        virtual Kind kind() const = 0;
    };

public:
    /// Definition for a box.
    struct Box : public Definition {
        Vector3d center = Vector3d::zero();
        Vector3d up_axis = Vector3d::y_axis();
        Vector3d orient_axis = Vector3d::x_axis();
        Color color = Color::grey();
        double height = 1;
        double width = 1;
        double depth = 1;
        double tileU = 1;
        double tileV = 1;

        Box() = default;
        Box(const Box&) = default;
        virtual ~Box() override;
        virtual Kind kind() const override { return Kind::BOX; }
    };

    /// Definition for a sphere.
    /// Spheres are created with poles in the vertical axis.
    struct Sphere : public Definition {
        Vector3d center = Vector3d::zero();
        double radius = 1;
        unsigned int rings = 12;    // latitude
        unsigned int segments = 24; // longitude
        double tileU = 1;
        double tileV = 1;

        Sphere() = default;
        Sphere(const Sphere&) = default;
        virtual ~Sphere() override;
        virtual Kind kind() const override { return Kind::SPHERE; }
    };

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Add a box to the factory's production list.
    void add(const Box& definition) { m_definitions.emplace_back(std::make_unique<Box>(std::move(definition))); }

    /// Add a sphere to the factory's production list.
    void add(const Sphere& definition) { m_definitions.emplace_back(std::make_unique<Sphere>(std::move(definition))); }

protected:
    /// Constructor.
    PrefabFactoryImpl() : m_definitions() {}

    /// Static production methods.
    static Product _produce(const Box& def);
    static Product _produce(const Sphere& def);

    /// Convert a study value into the appropriate OpenGL type.
    static void _convert(const Vector2d& in, Vector2f& out);
    static void _convert(const Vector2d& in, Vector2h& out);
    static void _convert(const Vector3d& in, Vector3f& out);
    static void _convert(const Vector3d& in, Vector3h& out);
    static void _convert(const Vector3d& in, Vector4f& out);
    static void _convert(const Vector3d& in, Vector4h& out);

    /// Error overloads for _convert to give helpful error messages.
    template<typename UNSUPPORTED_TYPE>
    static void _convert(const Vector3d&, UNSUPPORTED_TYPE&)
    {
        static_assert(always_false_t<UNSUPPORTED_TYPE>{}, "No conversion defined from Vector3d to UNSUPPORTED_TYPE");
    }
    template<typename UNSUPPORTED_TYPE>
    static void _convert(const Vector2d&, UNSUPPORTED_TYPE&)
    {
        static_assert(always_false_t<UNSUPPORTED_TYPE>{}, "No conversion defined from Vector2d to UNSUPPORTED_TYPE");
    }

    // fields ------------------------------------------------------------------------------------------------------- //
protected:
    /// All definitions added to the factory.
    std::vector<std::unique_ptr<Definition>> m_definitions;
};

} // namespace detail

// ================================================================================================================== //

/// Factory class for building new prefabs that are stored in a given library.
template<typename LIBRARY>
class PrefabFactory : public detail::PrefabFactoryImpl {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    using library_t = LIBRARY;

    using vertex_array_t = typename library_t::vertex_array_t;

    using VertexTraits = typename vertex_array_t::Traits;

    using VertexTraitIndices = std::make_index_sequence<std::tuple_size<VertexTraits>::value>;

public:
    using Vertex = typename vertex_array_t::Vertex;

    using InstanceData = typename library_t::instance_array_t::Vertex;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param library   Geometry library into which the factory produces prefab types.
    PrefabFactory(library_t& library) : m_library(library), m_studies(), m_indices() {}

    /// Produces a new prefab from the current state of the factory.
    /// @param name              Name of the new prefab.
    /// @throws runtime_error    If the name is already taken in the library.
    std::shared_ptr<PrefabType<InstanceData>> produce(std::string name)
    {
        if (m_library.has_prefab_type(name)) {
            NOTF_THROW(runtime_error, "Cannot produce new prefab type with existing name \"{}\"", name);
        }

        // build up the studies from the factory list
        for (const std::unique_ptr<Definition>& def : m_definitions) {
            switch (def->kind()) {
            case Kind::BOX:
                _ingest_product(_produce(*static_cast<Box*>(def.get())));
                break;
            case Kind::SPHERE:
                _ingest_product(_produce(*static_cast<Sphere*>(def.get())));
                break;
            }
        }

        // push the created vertices / indices into the library to create the new prefab type
        auto& library_vertices = static_cast<vertex_array_t*>(m_library.m_vertex_array.get())->m_vertices;
        const size_t prefab_offset = library_vertices.size();
        extend(library_vertices, _studies_to_vertices(VertexTraitIndices{}));

        auto& library_indices = static_cast<IndexArray<GLuint>*>(m_library.m_index_array.get())->buffer();
        const size_t prefab_size = m_indices.size();
        extend(library_indices, std::move(m_indices));

        std::shared_ptr<PrefabType<InstanceData>> prefab_type
            = PrefabType<InstanceData>::create(std::move(name), prefab_offset, prefab_size);
        m_library.m_prefab_types.push_back(prefab_type);

        // reset the factory
        m_definitions.clear();
        m_studies.clear();
        m_indices.clear();

        return prefab_type;
    }

private:
    /// During the production process, the factory creates new primitives and ingests them to form a larger prefab.
    void _ingest_product(Product&& product)
    {
        const size_t index_offset = m_studies.size();
        for (auto& index : product.indices) {
            index += index_offset;
        }
        extend(m_indices, std::move(product.indices));
        extend(m_studies, std::move(product.studies));
    }

    /// Convert a vector of Study objects to the vertex format specified by the VertexArray.
    template<std::size_t... TRAIT_INDEX>
    std::vector<Vertex> _studies_to_vertices(std::index_sequence<TRAIT_INDEX...>)
    {
        std::vector<Vertex> result(m_studies.size());
        for (size_t index = 0; index < m_studies.size(); ++index) {
            (_apply_study<TRAIT_INDEX>(typename std::tuple_element<TRAIT_INDEX, VertexTraits>::type::kind{},
                                       m_studies[index], result[index]),
             ...);
        }
        return result;
    }

    /// Takes a study and converts one of its values to store it in the given vertex.
    template<size_t index>
    static void _apply_study(const AttributeKind::Position, const Study& study, Vertex& vertex)
    {
        _convert(study.position, std::get<index>(vertex));
    }

    template<size_t index>
    static void _apply_study(const AttributeKind::Normal, const Study& study, Vertex& vertex)
    {
        _convert(study.normal, std::get<index>(vertex));
    }

    template<size_t index>
    static void _apply_study(const AttributeKind::Color, const Study& study, Vertex& vertex)
    {
        _convert(study.color, std::get<index>(vertex));
    }

    template<size_t index>
    static void _apply_study(const AttributeKind::TexCoord, const Study& study, Vertex& vertex)
    {
        _convert(study.tex_coord, std::get<index>(vertex));
    }

    template<size_t index, typename T>
    static void _apply_study(const T, const Study&, Vertex& vertex)
    {
        static_assert(std::is_pod<typename std::tuple_element<index, Vertex>::type>::value,
                      "PrefabFactory cannot produce a non-pod attribute type");
        memset(&std::get<index>(vertex), 0, sizeof(T)); // set unknown attribute to zero
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Geometry library into which the factory produces.
    library_t& m_library;

    /// All vertex sutdies of the geometry produced in the factory.
    std::vector<Study> m_studies;

    /// All indices of the geometry produced in the factory.
    std::vector<GLuint> m_indices;
};

NOTF_CLOSE_NAMESPACE
