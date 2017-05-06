#pragma once

#include "core/layout.hpp"

namespace notf {

class WindowLayout;
class Window;

using WindowLayoutPtr = std::shared_ptr<WindowLayout>;

/**********************************************************************************************************************/

/** WindowLayout Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class WindowLayoutIterator : public LayoutIterator {

public: // methods
    /** Constructor */
    WindowLayoutIterator(const WindowLayout* window_layout)
        : m_layout(window_layout)
    {
    }

    /** Destructor */
    virtual ~WindowLayoutIterator() = default;

    /** Advances the Iterator one step, returns the next Item or nullptr if the iteration has finished. */
    virtual Item* next() override;

private: // fields
    /** WindowLayout that is iterated over, is set to nullptr when the iteration has finished. */
    const WindowLayout* m_layout;
};

/**********************************************************************************************************************/

/** The WindowLayout is owned by a Window and root of all LayoutItems displayed within the Window. */
class WindowLayout : public Layout {

    friend class WindowLayoutIterator;
    friend class Window;

private: // factory
    /** Factory method. */
    static WindowLayoutPtr create(const std::shared_ptr<Window>& window);

    /** @param window   Window owning this RootWidget. */
    WindowLayout(const std::shared_ptr<Window>& window);

public: // methods
    /** Destructor. */
    virtual ~WindowLayout() override;

    /** Tests if a given Item is a child of this Item. */
    bool has_item(const ItemPtr& item) const;

    /** Checks if this Layout is empty or not. */
    bool is_empty() const { return m_controller.get() == nullptr; }

    /** Removes all Items from the Layout. */
    void clear();

    /** Sets a new Controller for the WindowLayout. */
    void set_controller(ControllerPtr controller);

    /** Removes a single Item from this Layout.
     * Does nothing, if the Item is not a child of this Layout.
     */
    virtual void remove_item(const ItemPtr& item) override;

    /** Returns the united bounding rect of all Items in the Layout. */
    virtual Aabrf get_content_aabr() const override;

    /** Returns an iterator that goes over all Items in this Layout in order from back to front. */
    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

    /** Returns the Window owning this WindowLayout. */
    std::shared_ptr<Window> get_window() const;

    /** Find all Widgets at a given position in the Window.
     * @param local_pos     Local coordinates where to look for a Widget.
     * @return              All Widgets at the given coordinate, ordered from front to back.
     */
    std::vector<Widget*> get_widgets_at(const Vector2f& screen_pos) const;

protected: // methods
    virtual void _relayout() override;

private: // methods
    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    virtual Claim _aggregate_claim() override;

private: // fields
    /** The Window containing this WindowLayout. */
    Window* m_window;

    /** The Window Controller. */
    ControllerPtr m_controller;
};

} // namespace notf
