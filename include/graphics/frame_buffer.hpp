#pragma once

#include <memory>
#include <variant>

#include "common/meta.hpp"

namespace notf {

DEFINE_SHARED_POINTERS(class, RenderBuffer)
DEFINE_SHARED_POINTERS(class, Texture)

//====================================================================================================================//

class RenderBuffer {
};


//====================================================================================================================//

class FrameBuffer {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    /// @brief Arguments used to initialize a Texture.
    struct Args {

        /// @brief Anisotropy factor - is only used if the anisotropic filtering extension is supported.
        /// A value <= 1 means no anisotropic filtering.
        float anisotropy = 1.0f;
    };
    
    // fields --------------------------------------------------------------------------------------------------------//
private:

    /// @brief Color target (is always a texture).    
    TexturePtr m_color_target;
    
    // @brief Depth target.
    RenderBufferPtr m_depth_target;
    
    // @brief Stencil target.
    RenderBufferPtr m_stencil_target;
};

} // namespace notf
