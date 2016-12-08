#pragma once

#include "core/item.hpp"
#include "utils/private_except_for_bindings.hpp"

namespace notf {

template <typename T>
class MakeSmartEnabler;

/**********************************************************************************************************************/

/// @brief Something drawn on the screen, potentially able to interact with the user.
class Widget : public Item {

    friend class MakeSmartEnabler<Widget>;

public: // methods
    /** Returns the Layout used to scissor this Widget.
     * Returns an empty shared_ptr, if no explicit scissor Layout was set or the scissor Layout has since expired.
     * In this case, the Widget is implicitly scissored by its parent Layout.
     */
    std::shared_ptr<Layout> get_scissor_layout() const { return m_scissor_layout.lock(); }

    virtual const Claim& get_claim() const override;

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    /** Returns a requested Property by name or nullptr if the Widget has no Property by that name. */
    AbstractProperty* get_property(const std::string& name);

    /** Tells the Window that its contents need to be redrawn. */
    void redraw() { _redraw(); }

    // clang-format off
private_except_for_bindings: // methods for MakeSmartEnabler<Widget>;
    /** @param state_machine     StateMachine of this Widget. Applies the default state. */
    explicit Widget(std::shared_ptr<StateMachine> state_machine);

private_except_for_bindings : // methods
    /** Stores the Python subclass object of this Widget, if it was created through Python. */
    void set_pyobject(PyObject* object) { _set_pyobject(object); }

    // clang-format on
protected: // methods
    /** Adds a new Property to this Widget.
     * @param name      Name of the Property, must be unique in this Widget.
     * @param value     Value of the Property, must be of a type supported by AbstractProperty.
     * @return          Iterator to the new Property.
     * @throw           std::runtime_error if the name is not unique.
     */
    template <typename T>
    PropertyMap::iterator _add_property(std::string name, const T value)
    {
        return add_property(m_properties, std::move(name), std::move(value));
    }

private: // methods
    virtual bool _redraw() override;

private: // fields
    /** Reference to a Layout used to 'scissor' this item.
     * Example: a scroll area 'scissors' children that overflow its size.
     * An empty pointer means that this item is scissored by its parent Layout.
     * An expired weak_ptr is treated like an empty pointer.
     */
    std::weak_ptr<Layout> m_scissor_layout;
};

} // namespace notf
