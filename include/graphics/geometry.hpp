#pragma once

#include <cstring>
#include <vector>

#include "common/half.hpp"
#include "common/vector2.hpp"
#include "common/vector4.hpp"
#include "graphics/vertex_array.hpp"

namespace notf {

namespace detail {

class GeometryFactoryImpl {

public: // types ******************************************************************************************************/
    //    /** Definition for a rectangular plane.
    //     * Default is a plane facing towards positive z (up).
    //     */
    //    struct RectangularPlaneDefinition {
    //        Vector4d center = Vector4d::zero();
    //        Vector4d axisX  = Vector4d::x_axis();
    //        Vector4d axisY  = Vector4d::y_axis();
    //        float sizeX     = 1;
    //        float sizeY     = 1;
    //        float tileU     = 1;
    //        float tileV     = 1;
    //    };

    //    /** Definition for a circular plane.
    //     * Default is a plane facing towards positive z.
    //     */
    //    struct CircularPlaneDefinition {
    //        Vector4d center       = Vector4d::zero();
    //        Vector4d upAxis       = Vector4d::y_axis();
    //        Vector4d orientAxis   = Vector4d::x_axis();
    //        float radius          = 1;
    //        unsigned int segments = 12;
    //        float tileU           = 1;
    //        float tileV           = 1;
    //    };

    /** Definition for a box. */
    struct BoxDefinition {
        Vector4d center      = Vector4d::zero();
        Vector4d up_axis     = Vector4d::y_axis();
        Vector4d orient_axis = Vector4d::x_axis();
        double height        = 1;
        double width         = 1;
        double depth         = 1;
        double tileU         = 1;
        double tileV         = 1;
    };

    //    /** Definition for a sphere.
    //     * Implemented from:
    //     *     http://stackoverflow.com/a/26710491/3444217
    //     * Spheres are always created with poles in the vertical axis.
    //     */
    //    struct SphereDefinition {
    //        Vector4d center       = Vector4d::zero();
    //        float radius          = 1;
    //        unsigned int rings    = 12; // latitude
    //        unsigned int segments = 24; // longitude
    //        float tileU           = 1;
    //        float tileV           = 1;
    //    };

    //    /** Definition for a cylinder.
    //     * Default is a cylinder along positive z.
    //     */
    //    struct CylinderDefinition {
    //        Vector4d center       = Vector4d::zero();
    //        Vector4d upAxis       = Vector4d::y_axis();
    //        Vector4d orientAxis   = Vector4d::x_axis();
    //        float radius          = .5;
    //        float height          = 1;
    //        unsigned int segments = 12;
    //        float tileU           = 1;
    //        float tileV           = 1;
    //    };

    //    /** Definition for a cone.
    //     * Default is a cone along positive y.
    //     */
    //    struct ConeDefinition {
    //        Vector4d center       = Vector4d::zero();
    //        Vector4d upAxis       = Vector4d::y_axis();
    //        Vector4d orientAxis   = Vector4d::x_axis();
    //        float radius          = .5;
    //        float height          = 1;
    //        unsigned int segments = 12;
    //        float tileU           = 1;
    //        float tileV           = 1;
    //    };

    //    /** Definition for a bone. */
    //    struct BoneDefinition {
    //        Vector4d center       = Vector4d::zero();
    //        Vector4d upAxis       = Vector4d::y_axis();
    //        Vector4d orientAxis   = Vector4d::x_axis();
    //        float radius          = .5;
    //        float height          = 2;
    //        float midHeight       = .5;
    //        unsigned int segments = 4;
    //        float tileU           = 1;
    //        float tileV           = 1;
    //    };

    //    /** Definition for a curved strip.
    //     * Default "rainbow" surface facing positive z.
    //     */
    //    struct CurvedStripDefinition {
    //        Vector4d start        = Vector4d(10, 0, 0);
    //        Vector4d axisX        = Vector4d::x_axis();
    //        Vector4d rotPivot     = Vector4d::zero();
    //        Vector4d rotAxis      = Vector4d::z_axis();
    //        float width           = 1;
    //        float angle           = 3.14159265359f;
    //        unsigned int segments = 32;
    //        float tileU           = 1;
    //        float tileV           = 10;
    //    };

protected: // types ***************************************************************************************************/
    /** Intermediate structure independent of the Vertex layout. */
    struct Study {
        Vector4d position;
        Vector4d normal;
        Vector2d tex_coord;
    };

protected: // methods *************************************************************************************************/
    /** Produce a sphere. */
    static std::vector<Study> _produce(const BoxDefinition& def);

    /** Convert a study value into the appropriate OpenGL type. */
    static void _convert(const Vector4d& in, std::array<float, 4>& out)
    {
        for (size_t i = 0; i < 4; ++i) {
            out[i] = static_cast<float>(in[i]);
        }
    }
    static void _convert(const Vector4d& in, std::array<half, 4>& out)
    {
        for (size_t i = 0; i < 4; ++i) {
            out[i] = half(static_cast<float>(in[i]));
        }
    }
};

} // namespace detail

//*********************************************************************************************************************/
//*********************************************************************************************************************/

/** Factory class for gradually building up and producing Geometry. */
template <typename VERTEX_ARRAY, ENABLE_IF_SUBCLASS(VERTEX_ARRAY, VertexArrayType)>
class GeometryFactory : public detail::GeometryFactoryImpl {

    using Traits = typename VERTEX_ARRAY::Traits;

    using Vertex = typename VERTEX_ARRAY::Vertex;

public: // methods ****************************************************************************************************/
    template <typename Indices = std::make_index_sequence<std::tuple_size<Traits>::value>>
    static std::vector<Vertex> produce() { return _adapt(_produce(BoxDefinition{}), Indices{}); }

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
        //        static_assert(std::is_same<typename std::tuple_element<index, Vertex>::type, Vector4d>::value,
        //                      "An Attribute trait of kind Position must be of type Vector4d");
        _convert(study.position, std::get<index>(vertex));
    }

    template <size_t index>
    static void _apply_study(const AttributeKind::TexCoord, const Study& study, Vertex& vertex)
    {
        static_assert(std::is_same<typename std::tuple_element<index, Vertex>::type, Vector2d>::value,
                      "An Attribute trait of kind TexCoord must be of type Vector2d");
        _convert(study.tex_coord, std::get<index>(vertex));
    }

    template <size_t index>
    static void _apply_study(const AttributeKind::Normal, const Study& study, Vertex& vertex)
    {
        //        static_assert(std::is_same<typename std::tuple_element<index, Vertex>::type, Vector4d>::value,
        //                      "An Attribute trait of kind Normal must be of type Vector4d");
        _convert(study.normal, std::get<index>(vertex));
    }

#if __cplusplus <= 201402L // use recursion up to C++14
    /** Find the correct study for each Vertex trait and apply it. */
    template <size_t FIRST_INDEX, size_t SECOND_INDEX, size_t... REST>
    static void _apply_studies(const Study& study, Vertex& vertex)
    {
        _apply_study<FIRST_INDEX>(typename std::tuple_element<FIRST_INDEX, Traits>::type::kind{}, study, vertex);
        _apply_studies<SECOND_INDEX, REST...>(study, vertex);
    }

    /** Recursion breaker for _apply_studies. */
    template <size_t LAST_INDEX>
    static void _apply_studies(const Study& study, Vertex& vertex)
    {
        _apply_study<LAST_INDEX>(typename std::tuple_element<LAST_INDEX, Traits>::type::kind{}, study, vertex);
    }
#endif

    /** Convert a vector of Study objects to the vertex format specified by the VertexArray. */
    template <std::size_t... TRAIT_INDEX>
    static std::vector<Vertex> _adapt(const std::vector<Study>& studies, std::index_sequence<TRAIT_INDEX...>)
    {
        std::vector<Vertex> result(studies.size());
        for (size_t index = 0; index < studies.size(); ++index) {
#if __cplusplus <= 201402L
            _apply_studies<TRAIT_INDEX...>(studies[index], result[index]);
#else // use fold expression from C++17 onwards
            (_apply_study<TRAIT_INDEX>(typename std::tuple_element<TRAIT_INDEX, Traits>::type::kind{},
                                       studies[index], result[index]),
             ...);
#endif
        }
        return result;
    }
};

} // namespace notf
