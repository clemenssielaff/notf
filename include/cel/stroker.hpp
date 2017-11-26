#pragma once

#include "common/forwards.hpp"

namespace notf {

// ===================================================================================================================//

/// @brief Manager for rendering 2D lines.
/// Manages the shader and -invokation, as well as the rendered vertices.
class Stroker {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(Stroker)

    Stroker();

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// @brief Shader used to render the lines.
    ShaderPtr m_shader;

    /// @brief Rendered vertices.
    VertexArrayTypePtr m_vertices;
};

} // namespace notf
