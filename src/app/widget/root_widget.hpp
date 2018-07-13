#pragma once

#include "app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// The root widget is the first node underneath the Scene's RootNode.
class RootWidget : public Widget {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    RootWidget(FactoryToken token, Scene& scene, valid_ptr<Node*> parent);

    /// Destructor.
    ~RootWidget() override;

    /// Sets a new child Widget at the top of the Widget hierarchy.
    /// @param args Arguments that are forwarded to the constructor of the child.
    template<class T, class... Args, typename = enable_if_t<std::is_base_of<Widget, T>::value>>
    NodeHandle<T> set_child(Args&&... args)
    {
        _clear_children();
        return _add_child<T>(std::forward<Args>(args)...);
    }

    /// Removes the child Widget, effectively clearing the Scene.
    void clear() { _clear_children(); }

    /// The Clipping rect of the Window.
    const Clipping& get_clipping_rect() const final { return m_clipping; }

private:
    /// Updates the Design of this Widget through the given Painter.
    void _paint(Painter&) const final {}

    /// Recursive implementation to find all Widgets at a given position in local space
    /// @param local_pos     Local coordinates where to look for a Widget.
    /// @return              All Widgets at the given coordinate, ordered from front to back.
    void _get_widgets_at(const Vector2f& local_pos, std::vector<valid_ptr<Widget*>>& result) const final;

    /// Called when the RootWidget's grant has changed (so basically, when the Window size has changed).
    void _on_grant_changed(const Size2f& new_grant);

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Clipping rect of the Window.
    Clipping m_clipping;
};

NOTF_CLOSE_NAMESPACE
