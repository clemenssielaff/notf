#pragma once

#include <memory>
#include <vector>

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
    /// \brief Destructor.
    ~Widget();

    void set_parent(std::shared_ptr<Widget> parent);

    /// \brief The Application-unique Handle of this Widget.
    Handle get_handle() const { return m_handle; }

protected: // methods
    /// \brief Value Constructor.
    explicit Widget(Handle handle);

private: // methods for Application
    /// \brief Factory function to create a new Widget instance.
    ///
    /// \param handle   Handle of the new Widget.
    static std::shared_ptr<Widget> make_widget(Handle handle);

private: // fields
    /// \brief Application-unique Handle of this Widget.
    Handle m_handle;

    /// \brief Framing of this Widget.
    FRAMING m_framing;

    /// \brief Parent widget.
    std::weak_ptr<Widget> m_parent;

    /// \brief All child widgets.
    std::vector<std::shared_ptr<Widget> > m_children;

    CALLBACKS(Widget)
};

} // namespace signal
