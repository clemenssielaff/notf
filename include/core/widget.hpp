#pragma once

#include <memory>
#include <vector>

#include "common/enummap.hpp"
#include "core/component.hpp"
#include "core/layout_item.hpp"

namespace notf {

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutItem {

public: // methods
    /// @brief Returns the Window containing this Widget (can be nullptr).
    std::shared_ptr<Window> get_window() const;

    /// @brief Checks if this Widget contains a Component of the given kind.
    /// @param kind Component kind to check for.
    bool has_component_kind(Component::KIND kind) const { return m_components.count(kind); }

    /// @brief Requests the Component of a given kind from this Widget.
    /// @return The requested Component, shared pointer is empty if this Widget has no Component of the requested kind.
    template <typename COMPONENT>
    std::shared_ptr<COMPONENT> get_component() const
    {
        auto it = m_components.find(get_kind<COMPONENT>());
        if (it == m_components.end()) {
            return {};
        }
        return std::static_pointer_cast<COMPONENT>(it->second);
    }

    /// @brief Attaches a new Component to this Widget.
    /// @param component    The Component to attach.
    ///
    /// Each Widget can only have one instance of each Component kind attached.
    void add_component(std::shared_ptr<Component> component);

    /// @brief Removes an existing Component of the given kind from this Widget.
    /// @param kind Kind of the Component to remove.
    ///
    /// If the Widget doesn't have the given Component kind, the call is ignored.
    void remove_component(Component::KIND kind);

    /// @brief Updates the Claim of this Widget.
    void set_claim(const Claim claim)
    {
        _set_claim(std::move(claim));
        if (_is_dirty()) {
            _update_parent_layout();
        }
    }

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    /// @brief Tells this Widget to redraw.
    void redraw() { _redraw(); }

public: // static methods
    /// @brief Factory function to create a new Widget instance.
    /// If an explicit handle is passed, it is assigned to the new Widget.
    /// This function will throw if the existing Handle is already taken.
    /// If no handle is passed, a new one is created.
    /// @param handle               [optional] Handle of the new widget.
    /// @throw std::runtime_error   If the Widget could not be created with the given Handle (or at all).
    /// @return The created Widget, pointer is empty on error.
    static std::shared_ptr<Widget> create(Handle handle = BAD_HANDLE);

protected: // methods
    /// @brief Value Constructor.
    /// @param handle   Handle of this Widget.
    explicit Widget(const Handle handle)
        : LayoutItem(handle)
        , m_components()
    {
    }

    virtual void _redraw() override;

    virtual void _relayout(const Size2f) override { _redraw(); }
    // TODO: is there really a functional difference between _redraw and _relayout?
    // I mean, a Layout will ONLY relayout, while a Widget will ONLY redraw

private: // fields
    /// @brief All components of this Widget.
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;
};

} // namespace notf
