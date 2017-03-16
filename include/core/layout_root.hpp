#pragma once

#include "core/layout.hpp"

namespace notf {

class LayoutRoot;
class Window;

/**********************************************************************************************************************/

/** LayoutRoot Iterator that goes through all items in a Layout in order, from back to front.
 * Iterators must be used up immediately after creation as they might be invalidated by any operation on their Layout.
 */
class LayoutRootIterator : public LayoutIterator {

    friend class LayoutRoot;

protected: // for LayoutRoot
    LayoutRootIterator(const LayoutRoot* layout_root)
        : m_layout(layout_root)
    {
    }

public: // methods
    virtual ~LayoutRootIterator() = default;

    /** Advances the Iterator one step, returns the next Item or nullptr if the iteration has finished. */
    virtual const Item* next() override;

private: // fields
    /** LayoutRoot that is iterated over, is set to nullptr when the iteration has finished. */
    const LayoutRoot* m_layout;
};

/**********************************************************************************************************************/

/** The Layout Root is owned by a Window and root of all LayoutItems displayed within the Window. */
class LayoutRoot : public Layout {

    friend class LayoutRootIterator;
    friend class Window;

protected: // methods
    /** @param window   Window owning this RootWidget. */
    explicit LayoutRoot(const std::shared_ptr<Window>& window);

public: // methods
    /** Returns the Window owning this LayoutRoot. */
    std::shared_ptr<Window> get_window() const;

    /** Sets a new Item at the LayoutRoot. */
    void set_controller(std::shared_ptr<Controller> controller);

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

    /** Find all Widgets at a given position in the Window.
     * @param local_pos     Local coordinates where to look for a Widget.
     * @return              All Widgets at the given coordinate, ordered from front to back.
     */
    std::vector<Widget*> get_widgets_at(const Vector2f& screen_pos) const;

protected: // methods
    virtual bool _update_claim() override { return false; }

    virtual void _remove_item(const Item*) override {}

    virtual void _relayout() override;

private: // methods
    /// @brief Returns the Layout contained in this LayoutRoot, may be invalid.
    Item* _get_item() const;

    virtual void _get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

private: // fields
    /** The Window containing this LayoutRoot. */
    Window* m_window;

    /** The Window Controller. */
    std::shared_ptr<Controller> m_controller;
};

} // namespace notf
