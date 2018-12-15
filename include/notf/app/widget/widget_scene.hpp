#pragma once

#include "notf/app/graph/scene.hpp"
#include "notf/app/widget/widget.hpp"
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
    template<class T, class... Args>
    void set_widget(Args&&... args) {
        _clear_children();
        return _create_child<T>(std::forward<Args>(args)...);
    }

    /// Returns the Widget at the top of the hierarchy in this Scene.
    AnyNodeHandle get_widget() const { return m_root_widget; }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The Widget underneath the root of this Scene.
    AnyNodeHandle m_root_widget;
};

NOTF_CLOSE_NAMESPACE
