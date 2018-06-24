#pragma once

#include "app/node_handle.hpp"
#include "app/scene.hpp"
#include "app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

class WidgetScene : public Scene {

    /// Constructor.
    /// @param token    Factory token provided by Scene::create.
    /// @param graph    The SceneGraph owning this Scene.
    /// @param name     Graph-unique, immutable name of the Scene.
    /// @throws scene_name_error    If the given name is not unique in the SceneGraph.
    WidgetScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name)
        : Scene(token, graph, std::move(name))
    {}

    /// Destructor.
    ~WidgetScene() override;

private:
    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    void _resize_view(Size2i) override;

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    bool _handle_event(Event& event) override;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The single Widget underneath the root of this Scene.
    NodeHandle<Widget> m_first;

    /// The lowest Widget to receive mouse events.
    /// When an Widget handles a mouse press event, it will also receive -move and -release events, even if the cursor
    /// is no longer within the Widget. May be empty.
    NodeHandle<Widget> m_mouse_focus;

    /// The lowest Widget to receive keyboard events. May be empty.
    NodeHandle<Widget> m_keyboard_focus;
};

NOTF_CLOSE_NAMESPACE
