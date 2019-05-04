#pragma once

#include "notf/app/graph/scene.hpp"
#include "notf/app/widget/any_widget.hpp"
#include "notf/app/widget/widget_visualizer.hpp"

NOTF_OPEN_NAMESPACE

// widget scene ===================================================================================================== //

// TODO: A way to prohibit nodes from changing parents (especially WidgetScene)
//      widget visualizer requires a graphics context, meaning it needs to stay in the same window

class WidgetScene : public Scene {

    // types ----------------------------------------------------------------------------------- //
public:
    /// WidgetScenes are only allowed to parent a single Widget.
    using allowed_child_types = std::tuple<AnyWidget>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor, constructs a full-screen, visible WidgetScene.
    /// @param parent   Parent of this Node.
    WidgetScene(valid_ptr<AnyNode*> parent);

    /// Sets a new Widget at the top of the hierarchy in this Scene.
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<class T, class... Args, class = std::enable_if_t<std::is_base_of_v<AnyWidget, T>>>
    auto set_widget(Args&&... args) {
        _clear_children();

        // create the new widget and resize it to fit the scene
        auto widget = _create_child<T>(this, std::forward<Args>(args)...).to_handle();
        m_root_widget = handle_cast<WidgetHandle>(widget);
        WidgetHandle::AccessFor<WidgetScene>::set_grant(m_root_widget, get<area>().get_size());

        return widget;
    }

    /// Returns the Widget at the top of the hierarchy in this Scene.
    AnyNodeHandle get_widget() const { return m_root_widget; }

    /// Outermost clipping rect, encompasses the entire Scene.
    const Aabrf& get_clipping_rect() const { return m_clipping; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The Widget underneath the root of this Scene.
    WidgetHandle m_root_widget;

    /// Outermost clipping rect, encompasses the entire Scene.
    Aabrf m_clipping;
};

// widget scene handle ============================================================================================== //

namespace detail {

template<>
struct NodeHandleInterface<WidgetScene> : public NodeHandleBaseInterface<WidgetScene> {

    using WidgetScene::get_widget;
    using WidgetScene::set_widget;
};

} // namespace detail

class WidgetSceneHandle : public NodeHandle<WidgetScene> {

    // methods --------------------------------------------------------------------------------- //
public:
    // use baseclass' constructors
    using NodeHandle<WidgetScene>::NodeHandle;

    /// Constructor from specialized base.
    WidgetSceneHandle(NodeHandle<WidgetScene>&& handle) : NodeHandle(std::move(handle)) {}
};

NOTF_CLOSE_NAMESPACE
