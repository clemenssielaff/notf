#pragma once

#include <memory>
#include <vector>

#include "core/layout_item.hpp"

namespace notf {

class State;
class StateMachine;

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutItem {

public: // methods
    /** Returns the current State of this Widget. Is always valid. */
    const State* get_state() const { return m_current_state; }

    /** Returns the StateMachine of this Widget. */
    std::shared_ptr<StateMachine> get_state_machine() const { return m_state_machine; }

    /** Returns the Layout used to scissor this Widget.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set or the scissor Layout has since expired.
     * In this case, the Widget is implicitly scissored by its parent Layout.
     */
    std::shared_ptr<Layout> get_scissor_layout() const { return m_scissor_layout.lock(); }

    virtual const Claim& get_claim() const override;

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    /** Tells the Window that its contents need to be redrawn. */
    void redraw() { _redraw(); }

public: // static methods
    /// @brief Factory function to create a new Widget instance.
    /// If an explicit handle is passed, it is assigned to the new Widget.
    /// This function will throw if the existing Handle is already taken.
    /// If no handle is passed, a new one is created.
    /// @param handle               [optional] Handle of the new widget.
    /// @throw std::runtime_error   If the Widget could not be created with the given Handle (or at all).
    /// @return The created Widget, pointer is empty on error.
    static std::shared_ptr<Widget> create(std::shared_ptr<StateMachine> state_machine, Handle handle = BAD_HANDLE);

protected: // methods
    /**
     * @param handle            Handle of this Widget.
     * @param state_machine     StateMachine of this Widget. Applies the default state.
     */
    Widget(const Handle handle, std::shared_ptr<StateMachine> state_machine);

private: // methods
    virtual bool _redraw() override;

private: // fields
    /** The StateMachine attached to this Widget. */
    std::shared_ptr<StateMachine> m_state_machine;

    /** Current State of the Widget. */
    const State* m_current_state;

    /** Reference to a Layout used to 'scissor' this item.
     * Example: a scroll area 'scissors' children that overflow its size.
     * An empty pointer means that this item is scissored by its parent Layout.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;
};

} // namespace notf
