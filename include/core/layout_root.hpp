#pragma once

#include "core/layout.hpp"

namespace notf {

template <typename T>
class MakeSmartEnabler;

class AbstractController;
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

    /** Advances the Iterator one step, returns the next Item or nullptr if the iteration has finished. */
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

    friend class LayoutRootIterator;
    friend class MakeSmartEnabler<LayoutRoot>;
    friend class Window;

public: // methods
    /// @brief Returns the Window owning this LayoutRoot.
    std::shared_ptr<Window> get_window() const;

    /// @brief Sets a new Item at the LayoutRoot.
    void set_item(std::shared_ptr<AbstractController> item);

    virtual bool get_widgets_at(const Vector2f local_pos, std::vector<Widget*>& result) override;

    virtual void set_render_layer(std::shared_ptr<RenderLayer>) override;

    virtual std::unique_ptr<LayoutIterator> iter_items() const override;

protected: // methods
    virtual bool _update_claim() override { return false; }

    virtual void _remove_item(const Item*) override {}

    virtual void _relayout() override;

private: // methods for MakeSmartEnabler<LayoutRoot>
    /** @param window   Window owning this RootWidget. */
    explicit LayoutRoot(const std::shared_ptr<Window>& window);

private: // methods
    /// @brief Returns the Layout contained in this LayoutRoot, may be invalid.
    Item* _get_item() const;

private: // fields
    /// @brief The Window containing this LayoutRoot.
    Window* m_window;
};

} // namespace notf
