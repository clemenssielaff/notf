#pragma once

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "common/devel.hpp"
#include "common/enummap.hpp"
#include "common/handle.hpp"
#include "common/log.hpp"
#include "common/signal.hpp"
#include "core/component.hpp"

namespace signal {

class Window;

/// \brief The Widget class
///
/// Widgets form a hierarchy.
/// Each Widget has a single parent (or not) and 0-n children.
/// Widgets without parents are not part of any hierarchy unless they are the root-Widget of a Window.
/// Each Window has its own Widget hierarchy with a single root-Widget at the top.
/// Widgets can be freely moved in the hierarchy, store its Handle to to keep a persistent reference to a Widget.
/// Using the Handle, you can even serialize and deserialize a hierarchy (which wouldn't work with pointers).
class Widget : public std::enable_shared_from_this<Widget> {

    friend class Window;

public: // enums
    /// \brief Framing is how a Widget is drawn in relation to its parent.
    enum class FRAMING {
        WITHIN,
        BEHIND,
        OVER,
    };

protected: // methods
    /// \brief Value Constructor.
    explicit Widget(Handle handle)
        : m_handle{std::move(handle)}
        , m_framing{FRAMING::WITHIN}
        , m_parent()
        , m_window(nullptr)
        , m_components()
        , m_children()
    {
    }

public: // methods
    /// no copy / assignment
    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    /// \brief Destructor.
    ~Widget();

    /// \brief Returns the parent Widget. If this Widget has no parent, the returned shared pointer is empty.
    std::shared_ptr<Widget> get_parent() const { return m_parent.lock(); }

    /// \brief Sets a new parent Widget.
    /// \param parent   New parent Widget.
    void set_parent(std::shared_ptr<Widget> parent);

    /// \brief The Application-unique Handle of this Widget.
    Handle get_handle() const { return m_handle; }

    /// \brief Returns the Window containing this Widget (can be nullptr).
    const Window* get_window() const { return m_window; }

    /// \brief Requests the Component of a given kind from this Widget.
    /// \return The requested Component, shared pointer is empty if this Widget has no Component of the requested kind.
    template <typename COMPONENT>
    std::shared_ptr<COMPONENT> get_component() const
    {
        auto it = m_components.find(get_kind<COMPONENT>());
        if (it == m_components.end()) {
            return {};
        }
        return std::static_pointer_cast<COMPONENT>(it->second);
    }

    /// \brief Checks if this Widget contains a Component of the given kind.
    /// \param kind Component kind to check for.
    bool has_component_kind(Component::KIND kind) const { return m_components.count(kind); }

    /// \brief Attaches a new Component to this Widget.
    /// \param component    The Component to attach.
    ///
    /// Each Widget can only have one instance of each Component kind attached.
    void add_component(std::shared_ptr<Component> component);

    /// \brief Removes an existing Component of the given kind from this Widget.
    /// \param kind Kind of the Component to remove.
    ///
    /// If the Widget doesn't have the given Component kind, the call is ignored.
    void remove_component(Component::KIND kind);

    /// \brief Draws this and all child widgets recursively.
    void redraw();

public: // static methods
    /// \brief Factory function to create a new Widget instance.
    /// \param handle   [optional] Handle of the new widget.
    /// \return The created Widget, pointer is empty on error.
    ///
    /// If an explicit handle is passed, it is assigned to the new Widget.
    /// This function will fail if the existing Handle is already taken.
    /// If no handle is passed, a new one is created.
    static std::shared_ptr<Widget> make_widget(Handle handle = BAD_HANDLE);

private: // methods for Window
    /// \brief Special factory function to create a Window's root Widget.
    static std::shared_ptr<Widget> make_root_widget(Window* window, Handle handle = BAD_HANDLE)
    {
        std::shared_ptr<Widget> widget = make_widget(handle);
        widget->m_window = window;
        return widget;
    }

private: // fields
    /// \brief Application-unique Handle of this Widget.
    Handle m_handle;

    /// \brief Framing of this Widget.
    FRAMING m_framing;

    /// \brief Parent Widget.
    std::weak_ptr<Widget> m_parent;

    /// \brief Window containing this Widget.
    Window* m_window;

    /// \brief All components of this Widget.
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;

    /// \brief All child widgets.
    std::vector<std::shared_ptr<Widget>> m_children;

    CALLBACKS(Widget)
};

} // namespace signal
