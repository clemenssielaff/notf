#pragma once

#include "core/layout.hpp"

namespace notf {

class LayoutRoot;
class Window;

/**********************************************************************************************************************/

/**
 * @brief LayoutRoot Iterator that goes through all items in a Layout in order, from back to front.
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

    /** Advances the Iterator one step, returns the next LayoutItem or nullptr if the iteration has finished. */
    virtual const Item* next() override;

private: // fields
    /** LayoutRoot that is iterated over, is set to nullptr when the iteration has finished. */
    const LayoutRoot* m_layout;
};

/**********************************************************************************************************************/

/*
 * @brief The Layout Root is owned by a Window and root of all LayoutItems displayed within the Window.
 */
class LayoutRoot : public Layout {

    friend class Window;
    friend class LayoutRootIterator;

public: // methods
    /// @brief Returns the Window owning this LayoutRoot.
    std::shared_ptr<Window> get_window() const;

    /// @brief Sets a new Item at the LayoutRoot.
    void set_item(std::shared_ptr<Item> item);

    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) override;

    virtual void set_render_layer(std::shared_ptr<RenderLayer>) override;

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

protected: // methods
    /** @param window   Window owning this RootWidget. */
    explicit LayoutRoot(const std::shared_ptr<Window>& window);

    virtual bool _update_claim() override { return false; }

    virtual void _remove_item(const Item*) override {}

    virtual void _relayout() override;

private: // methods
    /// @brief Returns the Layout contained in this LayoutRoot, may be invalid.
    Item* _get_item() const;

private: // fields
    /// @brief The Window containing this LayoutRoot.
    Window* m_window;
};

} // namespace notf
