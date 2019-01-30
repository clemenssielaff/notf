#pragma once

#include "notf/common/aabr.hpp"

#include "notf/app/graph/node.hpp"

NOTF_OPEN_NAMESPACE

// scene policy ===================================================================================================== //

namespace detail {
namespace scene_policy {

// properties =================================================================

/// Area of this Scene when not fullscreen.
struct Area {
    using value_t = Aabri;
    static constexpr ConstString name = "area";
    static constexpr value_t default_value = Aabri::zero();
    static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::INVISIBILE;
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
class Scene : public Node<detail::scene_policy::ScenePolicy> {

    friend SceneHandle;

    // types ----------------------------------------------------------------------------------- //
private:
    /// Compile time Node base type.
    using super_t = Node<detail::scene_policy::ScenePolicy>;

public:
    /// Property names.
    static constexpr const ConstString& area = detail::scene_policy::Area::name;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor, constructs a full-screen, visible Scene.
    /// @param parent   Parent of this Node.
    Scene(valid_ptr<AnyNode*> parent);

    /// Destructor.
    ~Scene();

    /// (Re-)Defines the Visualizer to use for drawing this Scene.
    void set_visualizer(VisualizerPtr visualizer);
    // TODO:NOW this is a hacky workaround at best. Since we can re-parent Nodes, a Scene requires a Visualizer
    // and the visualizer instance in turn relies on a single Window, we need some way to make sure that the
    // Scene re-creates its Visualizer, should it (or any of its parents) move to another window.
    // One way to deal with this would be to forbid reparenting of Scenes ... but that would mean also forbidding to
    // reparent any Node that could in turn parent a scene ... so basically getting rid of re-parenting completely.
    // Or The Scene (or any Node?) gets a virtual `_on_reparented()` method that gets called whenever the node is
    // reparented, and then the Scene would recreate a new Visualizer, maybe using a virtual factory on the old
    // instance?

    /// Whether the Scene is the direct child of a Window Node (a "Window Scene") or nested within another Scene.
    bool is_window_scene() const;

private:
    /// Draw the Scene.
    void _draw();

    // fields --------------------------------------------------------------------------------------------------- //
private:
    /// Visualizer that draws the Scene.
    VisualizerPtr m_visualizer;
};

// scene handle ===================================================================================================== //

namespace detail {

template<>
struct NodeHandleInterface<Scene> : public NodeHandleBaseInterface<Scene> {

    using Scene::is_window_scene;
};

} // namespace detail

class SceneHandle : public NodeHandle<Scene> {

    friend Accessor<SceneHandle, detail::RenderManager>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(SceneHandle);

    // methods --------------------------------------------------------------------------------- //
public:
    // use baseclass' constructors
    using NodeHandle<Scene>::NodeHandle;

    /// Constructor from specialized base.
    template<class Derived, class = std::enable_if_t<std::is_base_of_v<Scene, Derived>>>
    SceneHandle(NodeHandle<Derived>&& handle) : NodeHandle<Scene>(std::move(handle)) {}

private:
    /// Draws the Scene.
    void _draw() const { _get_node()->_draw(); }
};

// scene handle accessors =========================================================================================== //

template<>
class Accessor<SceneHandle, detail::RenderManager> {
    friend detail::RenderManager;

    /// Draws the Scene.
    static void draw(const SceneHandle& scene) { scene._draw(); }
};

NOTF_CLOSE_NAMESPACE
