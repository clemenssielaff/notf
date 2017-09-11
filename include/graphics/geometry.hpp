#pragma once

#include <cstring>
#include <vector>

#include "common/vector2.hpp"
#include "common/vector4.hpp"
#include "graphics/vertex_buffer.hpp"

namespace notf {

/** Factory class for gradually building up and producing Geometry. */
template <typename VERTEX_BUFFER, ENABLE_IF_SUBCLASS(VERTEX_BUFFER, VertexBufferType)>
class GeometryFactory {

    using Traits = typename VERTEX_BUFFER::Traits;

    using Vertex = typename VERTEX_BUFFER::Vertex;

    /** Intermediate structure independent of the Vertex layout. */
    struct Study {
        Vector4f position;
        Vector4f normal;
        Vector2f tex_coord;
    };

public: // types ******************************************************************************************************/
    /** Definition for a rectangular plane.
     * Default is a plane facing towards positive z (up).
     */
    struct RectangularPlaneDefinition {
        Vector4f center = Vector4f::zero();
        Vector4f axisX  = Vector4f::x_axis();
        Vector4f axisY  = Vector4f::y_axis();
        float sizeX     = 1;
        float sizeY     = 1;
        float tileU     = 1;
        float tileV     = 1;
    };

    /** Definition for a circular plane.
     * Default is a plane facing towards positive z.
     */
    struct CircularPlaneDefinition {
        Vector4f center       = Vector4f::zero();
        Vector4f upAxis       = Vector4f::y_axis();
        Vector4f orientAxis   = Vector4f::x_axis();
        float radius          = 1;
        unsigned int segments = 12;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a box. */
    struct BoxDefinition {
        Vector4f center     = Vector4f::zero();
        Vector4f upAxis     = Vector4f::y_axis();
        Vector4f orientAxis = Vector4f::x_axis();
        float height        = 1;
        float width         = 1;
        float depth         = 1;
        float tileU         = 1;
        float tileV         = 1;
    };

    /** Definition for a sphere.
     * Implemented from:
     *     http://stackoverflow.com/a/26710491/3444217
     * Spheres are always created with poles in the vertical axis.
     */
    struct SphereDefinition {
        Vector4f center       = Vector4f::zero();
        float radius          = 1;
        unsigned int rings    = 12; // latitude
        unsigned int segments = 24; // longitude
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a cylinder.
     * Default is a cylinder along positive z.
     */
    struct CylinderDefinition {
        Vector4f center       = Vector4f::zero();
        Vector4f upAxis       = Vector4f::y_axis();
        Vector4f orientAxis   = Vector4f::x_axis();
        float radius          = .5;
        float height          = 1;
        unsigned int segments = 12;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a cone.
     * Default is a cone along positive y.
     */
    struct ConeDefinition {
        Vector4f center       = Vector4f::zero();
        Vector4f upAxis       = Vector4f::y_axis();
        Vector4f orientAxis   = Vector4f::x_axis();
        float radius          = .5;
        float height          = 1;
        unsigned int segments = 12;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a bone. */
    struct BoneDefinition {
        Vector4f center       = Vector4f::zero();
        Vector4f upAxis       = Vector4f::y_axis();
        Vector4f orientAxis   = Vector4f::x_axis();
        float radius          = .5;
        float height          = 2;
        float midHeight       = .5;
        unsigned int segments = 4;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a curved strip.
     * Default "rainbow" surface facing positive z.
     */
    struct CurvedStripDefinition {
        Vector4f start        = Vector4f(10, 0, 0);
        Vector4f axisX        = Vector4f::x_axis();
        Vector4f rotPivot     = Vector4f::zero();
        Vector4f rotAxis      = Vector4f::z_axis();
        float width           = 1;
        float angle           = 3.14159265359f;
        unsigned int segments = 32;
        float tileU           = 1;
        float tileV           = 10;
    };

public: // methods ****************************************************************************************************/
    template <typename Indices = std::make_index_sequence<std::tuple_size<Traits>::value>>
    static std::vector<Vertex> produce()
    {
        std::vector<Study> studies;
        studies.reserve(3);
        studies.emplace_back(Study{Vector4f(-1, 0), Vector4f(-1, 0, 0), Vector2f(0, 0)});
        studies.emplace_back(Study{Vector4f(0, 1), Vector4f(0, 1, 0), Vector2f(0.5, 1)});
        studies.emplace_back(Study{Vector4f(1, 0), Vector4f(1, 0, 0), Vector2f(1, 0)});

        return _apply_studies(studies, Indices{});
    }

private: // methods ***************************************************************************************************/
    template <size_t index, typename T>
    static void _apply_study(const T, const Study&, Vertex& vertex)
    {
        static_assert(std::is_pod<typename std::tuple_element<index, Vertex>::type>::value,
                      "GeometryFactory cannot produce a non-pod attribute type");
        memset(&std::get<index>(vertex), 0, sizeof(T)); // set unknown attribute to zero
    }

    template <size_t index>
    static void _apply_study(const AttributeKind::Position, const Study& study, Vertex& vertex)
    {
        static_assert(std::is_same<typename std::tuple_element<index, Vertex>::type, Vector4f>::value,
                      "An Attribute trait of kind Position must be of type Vector4f");
        std::get<index>(vertex) = study.position;
    }

    template <size_t index>
    static void _apply_study(const AttributeKind::TexCoord, const Study& study, Vertex& vertex)
    {
        static_assert(std::is_same<typename std::tuple_element<index, Vertex>::type, Vector2f>::value,
                      "An Attribute trait of kind TexCoord must be of type Vector2f");
        std::get<index>(vertex) = study.tex_coord;
    }

    template <size_t index>
    static void _apply_study(const AttributeKind::Normal, const Study& study, Vertex& vertex)
    {
        static_assert(std::is_same<typename std::tuple_element<index, Vertex>::type, Vector4f>::value,
                      "An Attribute trait of kind Normal must be of type Vector4f");
        std::get<index>(vertex) = study.normal;
    }

    template <std::size_t... TRAIT_INDEX>
    static std::vector<Vertex> _apply_studies(const std::vector<Study>& studies, std::index_sequence<TRAIT_INDEX...>)
    {
        std::vector<Vertex> result(studies.size());
        for (size_t index = 0; index < studies.size(); ++index) {
            (_apply_study<TRAIT_INDEX>(typename std::tuple_element<TRAIT_INDEX, Traits>::type::kind{},
                                       studies[index], result[index]),
             ...);
        }
        return result;
    }
};

} // namespace notf
