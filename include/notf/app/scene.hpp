#pragma once

#include "notf/common/aabr.hpp"

#include "notf/app/node_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// scene policy ===================================================================================================== //

namespace detail {
namespace scene_policy {

// properties =================================================================

/// Area of this Scene when not fullscreen.
struct Area {
    using value_t = Aabri;
    static constexpr StringConst name = "area";
    static constexpr value_t default_value = Aabri::zero();
    static constexpr bool is_visible = true;
};

// policy =====================================================================

struct ScenePolicy {
    using properties = std::tuple< //
        Area                       //
        >;                         //

    using slots = std::tuple< //
        >;                    //

    using signals = std::tuple< //
        >;                      //
};

} // namespace scene_policy
} // namespace detail

// scene ============================================================================================================ //

/// Scenes are screen-axis-aligned quads that are drawn into a framebuffer (if the Scene is nested within another) or
/// directly into the screen buffer (a "Window Scene" owned directly by a Window).
/// The contents of a Scene are clipped to its area. The Scene's Visualizer can query the size of this area using
/// `GraphicsContext::render_area().size()` when drawing.
class Scene : public CompileTimeNode<detail::scene_policy::ScenePolicy> {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Compile time Node base type.
    using super_t = CompileTimeNode<detail::scene_policy::ScenePolicy>;

public:
    /// Property names.
    static constexpr const StringConst& area = detail::scene_policy::Area::name;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor, constructs a full-screen, visible Scene.
    /// @param parent   Parent of this Node.
    /// @param visualizer   Visualizer that draws the Scene.
    Scene(valid_ptr<Node*> parent, valid_ptr<VisualizerPtr> visualizer);

    /// Whether the Scene is the direct child of a Window Node (a "Window Scene") or nested within another Scene.
    bool is_window_scene() const;

    /// Draw the Scene.
    void draw();

    // fields --------------------------------------------------------------------------------------------------- //
private:
    /// Visualizer that draws the Scene.
    VisualizerPtr m_visualizer;
};

// scene handle ===================================================================================================== //

namespace detail {

template<>
struct NodeHandleInterface<Scene> : public NodeHandleBaseInterface<Scene> {};

} // namespace detail

class SceneHandle : public TypedNodeHandle<Scene> {

    // methods --------------------------------------------------------------------------------- //
public:
    // use baseclass' constructors
    using TypedNodeHandle<Scene>::TypedNodeHandle;
};

NOTF_CLOSE_NAMESPACE
