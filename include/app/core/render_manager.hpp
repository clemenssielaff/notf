#pragma once

#include <vector>

#include "graphics/core/frame_buffer.hpp"
#include "graphics/engine/post_effect.hpp"

namespace notf {

// The Render manager has a STATE that defines how to render a frame.
// At the top level you have a list of PLATES.
// Each plate is a full-screen quad, with an optional stack of EFFECTS, post-process shaders applied to their texture
// (like blur).  A plate refers to a render TARGET, a 2D image of arbitrary size.  Targets are created by one-n PRODUCERS,
// and can also be consumed by producers to create new Targets.  Targets can be dirty, in which case they have to be
// "cleaned" (reproduced) before their plates can be rendered.  A producer is any object that creates a target:
//    The PLOTTER is a producer
//    A producer which generates a 2D image through a shader (fog, for example)
//    A 3D object renderer

class RenderManager {
    // types ---------------------------------------------------------------------------------------------------------//
public:
    struct Plate {
        std::vector<PostEffectPtr> effects;


    };

    /// Complete state of the Render Buffer.
    struct State {};
};

} // namespace notf
