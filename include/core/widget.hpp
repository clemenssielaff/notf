#pragma once

#include <memory>
#include <vector>

#include "common/enummap.hpp"
#include "core/component.hpp"
#include "core/layout_object.hpp"

namespace signal {

class Window;

/// \brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutObject {

public: // methods
    /// \brief Returns the Window containing this Widget (can be nullptr).
    std::shared_ptr<Window> get_window() const;

    /// \brief Checks if this Widget contains a Component of the given kind.
    /// \param kind Component kind to check for.
    bool has_component_kind(Component::KIND kind) const { return m_components.count(kind); }

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
    virtual void redraw() override;

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2&) const override { return {}; } // TODO: Widget::get_widget_at

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Handle of this Widget.
    explicit Widget(const Handle handle)
        : LayoutObject(handle)
        , m_components()
    {
    }

public: // static methods
    /// \brief Factory function to create a new Widget instance.
    /// \param handle   [optional] Handle of the new widget.
    /// \return The created Widget, pointer is empty on error.
    ///
    /// If an explicit handle is passed, it is assigned to the new Widget.
    /// This function will fail if the existing Handle is already taken.
    /// If no handle is passed, a new one is created.
    static std::shared_ptr<Widget> create(Handle handle = BAD_HANDLE)
    {
        return create_item<Widget>(handle);
    }

private: // fields
    /// \brief All components of this Widget.
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;
};

} // namespace signal
