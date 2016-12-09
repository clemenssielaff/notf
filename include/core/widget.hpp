#pragma once

#include "core/layout_item.hpp"
#include "utils/private_except_for_bindings.hpp"

namespace notf {

template <typename T>
class MakeSmartEnabler;

/**********************************************************************************************************************/

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public LayoutItem {

    friend class MakeSmartEnabler<Widget>;

public: // methods
    /** Returns the Layout used to scissor this Widget.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set or the scissor Layout has since expired.
     * In this case, the Widget is implicitly scissored by its parent Layout.
     */
    std::shared_ptr<Layout> get_scissor_layout() const { return m_scissor_layout.lock(); }

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    /** Sets a new Claim for this Widget.
     * @return  True, iff the currenct Claim was changed.
     */
    bool set_claim(const Claim claim);

    /** Tells the Window that its contents need to be redrawn. */
    void redraw() { _redraw(); }

    // clang-format off
private_except_for_bindings: // methods for MakeSmartEnabler<Widget>;
    explicit Widget();

private_except_for_bindings : // methods
    /** Stores the Python subclass object of this Widget, if it was created through Python. */
    void set_pyobject(PyObject* object) { _set_pyobject(object); }

    // clang-format on
private: // fields
    /** Reference to a Layout used to 'scissor' this item.
     * Example: a scroll area 'scissors' children that overflow its size.
     * An empty pointer means that this item is scissored by its parent Layout.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;
};

} // namespace notf
