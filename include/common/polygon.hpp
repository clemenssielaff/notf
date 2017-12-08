#pragma once

#include <iosfwd>

#include "common/vector2.hpp"

namespace notf {

namespace detail {

//====================================================================================================================//

/// @brief Baseclass for Polygons.
template<typename REAL>
struct Polygon {

    /// @brief Vector type.
    using vector_t = RealVector2<REAL>;

    /// @brief Element type.
    using element_t = typename vector_t::element_t;

    // fields --------------------------------------------------------------------------------------------------------//
    /// @brief Vertices of this Polygon.
    std::vector<vector_t> vertices;

    // methods -------------------------------------------------------------------------------------------------------//
    /// @brief Default constructor.
    Polygon() = default;
};

} // namespace detail

//====================================================================================================================//

using Polygonf = detail::Polygon<float>;

//====================================================================================================================//

/// @brief Prints the contents of a Triangle into a std::ostream.
/// @param os       Output stream, implicitly passed with the << operator.
/// @param polygon  Polygon to print.
/// @return Output stream for further output.
std::ostream& operator<<(std::ostream& out, const Polygonf& polygon);

} // namespace notf

//====================================================================================================================//

namespace std {

/// @brief std::hash specialization for Triangle.
template<typename REAL>
struct hash<notf::detail::Polygon<REAL>> {
    size_t operator()(const notf::detail::Polygon<REAL>& polygon) const
    {
        auto result = notf::hash(static_cast<size_t>(notf::detail::HashID::POLYGON));
        for (const auto& vertex : polygon.vertices) {
            notf::hash_combine(result, vertex);
        }
        return result;
    }
};

} // namespace std
