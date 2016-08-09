#pragma once

#include <array>
#include <memory>
#include <vector>

#include "app/component.hpp"
#include "common/devel.hpp"
#include "common/handle.hpp"
#include "common/signal.hpp"

namespace signal {

/// \brief The Widget class
///
/// Widgets form a hierarchy.
/// Each Widget has a single parent (or not) and 0-n children.
/// Widgets without parents are not part of any hierarchy unless they are the root-Widget of a Window.
/// Each Window has its own Widget hierarchy with a single root-Widget at the top.
/// Widgets can be freely moved in the hierarchy, store its Handle to to keep a persistent reference to a Widget.
/// Using the Handle, you can even serialize and deserialize a hierarchy (which wouldn't work with pointers).
class Widget {

    friend class Application;

public: // enums
    /// \brief Framing is how a Widget is drawn in relation to its parent.
    enum class FRAMING {
        WITHIN,
        BEHIND,
        OVER,
    };

public: // methods
    /// no copy / assignment
    Widget(const Widget&) = delete;
    Widget& operator=(Widget const&) = delete;

    /// \brief Destructor.
    ~Widget();

    /// \brief Returns the parent Widget. If this Widget has no parent, the returned shared pointer is empty.
    std::shared_ptr<Widget> get_parent() const { return m_parent.lock(); }

    /// \brief Sets a new parent Widget.
    ///
    /// \param parent   New parent Widget.
    void set_parent(std::shared_ptr<Widget> parent);

    /// \brief The Application-unique Handle of this Widget.
    Handle get_handle() const { return m_handle; }

    /// \brief Checks if this Object contains a Component of the given kind.
    ///
    /// \param kind Component kind to check for.
    bool has_component_kind(Component::KIND kind) const { return bool(get_component(kind)); }

    /// Requests the Component of a given kind from this Widget.
    ///
    /// \return The requested Component, shared pointer is empty if this Widget has no Component of the requested kind.
    std::shared_ptr<Component> get_component(Component::KIND kind) const
    {
        return m_components.at(static_cast<size_t>(to_number(kind)));
    }

    /// \brief Attaches a new Component to this Object.
    ///
    /// Each Object can only have one instance of each Component kind attached.
    /// The previous Component of the given kind is returned.
    /// The shared pointer is empty if the Widget had no previous Component of the given kind.
    ///
    /// \param kind The kind of Component to attach to this Object.
    ///
    /// \return The previous Component of the new Component's type - may be empty.
    std::shared_ptr<Component> set_component(std::shared_ptr<Component> component);

public: // static methods
    /// \brief Factory function to create a new Widget instance.
    ///
    /// If an explicit handle is passed, it is assigned to the new Widget.
    /// This function will fail if the existing Handle is already taken.
    /// If no handle is passed, a new one is created.
    ///
    /// \param handle   [optional] Handle of the new widget.
    ///
    /// \return The created Widget, pointer is empty on error.
    static std::shared_ptr<Widget> make_widget(Handle handle = BAD_HANDLE);

protected: // methods
    /// \brief Value Constructor.
    explicit Widget(Handle handle);

private: // fields
    /// \brief Application-unique Handle of this Widget.
    Handle m_handle;

    /// \brief Framing of this Widget.
    FRAMING m_framing;

    /// \brief Parent widget.
    std::weak_ptr<Widget> m_parent;

    /// \brief All components of this Widget.
    std::array<std::shared_ptr<Component>, Component::get_count()> m_components;

    /// \brief All child widgets.
    std::vector<std::shared_ptr<Widget> > m_children;

    CALLBACKS(Widget)
};

} // namespace signal
