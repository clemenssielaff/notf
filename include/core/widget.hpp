#pragma once

#include <memory>
#include <vector>

#include "common/enummap.hpp"
#include "core/component.hpp"
#include "core/layout_item.hpp"
#include "core/state.hpp"

#ifndef SIGNAL_LOG_LEVEL
#define SIGNAL_LOG_LEVEL 0 // log all messages if no value for SIGNAL_LOG_LEVEL was given
#endif

namespace notf {

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutItem {

public: // methods
    /// @brief Returns the Window containing this Widget (can be nullptr).
    std::shared_ptr<Window> get_window() const;

/** Returns the current State of this Widget (may be nullptr). */
#if SIGNAL_LOG_LEVEL > 3 // SIGNAL_LOG_LEVEL_WARNING
    const State* get_current_state() const
    {
        return m_current_state;
    }
#else
    const State* get_current_state() const;
#endif

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
        , m_state_machine()
        , m_current_state(nullptr)
    {
    }

    virtual void _redraw() override;

    virtual void _relayout(const Size2f) override { _redraw(); }
    // TODO: is there really a functional difference between _redraw and _relayout?
    // I mean, a Layout will ONLY relayout, while a Widget will ONLY redraw

private: // fields
    /// @brief All components of this Widget.
    EnumMap<Component::KIND, std::shared_ptr<Component>> m_components;

    /** The StateMachine attached to this Widget. */
    std::shared_ptr<StateMachine> m_state_machine;

    /** Current State of the Widget. */
    const State* m_current_state;
};

} // namespace notf
