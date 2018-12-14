#pragma once

#include "notf/app/scene.hpp"
#include "notf/app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// widget scene ===================================================================================================== //

class WidgetScene : public Scene {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(WidgetScene);

    /// Constructor.
    /// @param token    Factory token provided by Scene::create.
    /// @param graph    The SceneGraph owning this Scene.
    /// @param name     Graph-unique, immutable name of the Scene.
    /// @throws scene_name_error    If the given name is not unique in the SceneGraph.
    WidgetScene(FactoryToken token, const valid_ptr<SceneGraphPtr>& graph, std::string name);

    /// Destructor.
    ~WidgetScene() override;

    /// Sets a new Widget at the top of the hierarchy in this Scene.
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<class T, class... Args, typename = std::enable_if_t<std::is_base_of<Widget, T>::value>>
    void set_widget(Args&&... args) {
        m_root_widget->set_child<T>(std::forward<Args>(args)...);
    }

    /// Returns the Widget at the top of the hierarchy in this Scene.
    template<class T, typename = std::enable_if_t<std::is_base_of<Widget, T>::value>>
    NodeHandle<T> get_widget() {
        return m_root_widget->get_child<T>(0);
    }

private:
    /// Resizes the view on this Scene.
    /// @param size Size of the view in pixels.
    void _resize_view(Size2i) override;

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    bool _handle_event(Event& event) override;

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// The RootWidget underneath the root of this Scene.
    NodeHandle<RootWidget> m_root_widget;
};

NOTF_CLOSE_NAMESPACE
