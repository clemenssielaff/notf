#pragma once


#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>


#include "core/component.hpp"
#include "utils/smart_enabler.hpp"

namespace signal {

class InternalLayout;
class Layout;
class LayoutItem;
class Widget;
class Vector2;

/// \brief Layouts Component.
/// Is made up of one or several Layout specializations, each accessible by name.
///
/// A LayoutComponent is special in the way that there is a 1:1 relationship between this Component and a Widget
/// (usually, Components can be shared by multiple Widgets).
/// However, a Widget may not have a LayoutComponent at all, which is why it is not an integral part of the Widget and
/// therefore a Component.
///
class LayoutComponent : public Component {

public: // methods



    /// \brief Creates a new Layout, owned by this Component.
    /// \return Pointer to the created Layout.
    ///
    /// Use as follows:
    ///     std::shared_ptr<MyLayout> = layout_component->create_layout<MyLayout>(arguments);
    /// where MyLayout is a specialization of Layout (otherwise, the template will not compile).
    template <typename LAYOUT, typename... ARGS, typename = typename std::enable_if<std::is_base_of<Layout, LAYOUT>::value>::type>
    std::shared_ptr<LAYOUT> create_layout(ARGS&&... args)
    {
        std::shared_ptr<LAYOUT> layout = std::make_shared<MakeSmartEnabler<LAYOUT>>(args...);
        m_layouts.emplace(layout);
        return layout;
    }

    /// \brief Sets the internal layout of this Component.
    /// If the internal layout is already defined, the old one is replaced.
    /// If the given Layout was not created by this Component, the call fails without changing the internal layout.
    /// If the given Layout pointer is empty, the internal layout is removed without being replaced.
    void set_internal_layout(std::shared_ptr<Layout> layout);

    /// \brief Redraws the Widget registered with this Component.
    void redraw_widget() { redraw_widgets(); }

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutComponent();

private: // fields
    /// \brief All Layouts of this Layout Component, accessible by name.
    std::set<std::shared_ptr<Layout>> m_layouts;

    /// \brief The internal layout.
    std::unique_ptr<InternalLayout> m_internal_layout;
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Root layout item, owned directly by a LayoutComponent.
/// Has a single or no child LayoutItem.
class InternalLayout : public LayoutItem {

    friend class LayoutComponent;

public: // methods
    /// \brief Returns the horizontal size range of this LayoutItem.
    virtual const SizeRange& get_horizontal_size() const override;

    /// \brief Returns the vertical size range of this LayoutItem.
    virtual const SizeRange& get_vertical_size() const override;

    /// \brief Returns true iff this LayoutItem is visible, false if it ishidden.
    virtual bool is_visible() const override
    {
        if (m_layout) {
            return m_layout->is_visible();
        }
        return false;
    }

    /// \brief Tells the containing layout to redraw (potentially cascading up the ancestry).
    virtual void redraw() const override
    {
        m_layout_component->redraw_widget();
    }

protected: // methods for LayoutComponent
    /// \brief Value Constructor.
    explicit InternalLayout(LayoutComponent* layout_component)
        : LayoutItem(nullptr)
        , m_layout_component(layout_component)
        , m_layout()
    {
    }

    /// \brief Sets a new internal Layout Item.
    void set_layout(std::shared_ptr<LayoutItem> layout) { m_layout = std::move(layout); }

private: // fields
    /// \brief LayoutComponent owning this Layout, is guaranteed to exist for the lifetime of this object.
    LayoutComponent* m_layout_component;

    /// \brief Layout Item contained in this Layout.
    std::shared_ptr<LayoutItem> m_layout;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief A container for a Widget in a Layout.
class LayoutWidget : public LayoutItem {

public: // methods
    /// \brief Virtual destructor.
    virtual ~LayoutWidget();

    /// \brief Returns the horizontal size range of this LayoutItem.
    virtual const SizeRange& get_horizontal_size() const = 0;

    /// \brief Returns the vertical size range of this LayoutItem.
    virtual const SizeRange& get_vertical_size() const = 0;

    /// \brief Returns true iff this LayoutItem is visible, false if it ishidden.
    virtual bool is_visible() const = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the ancestry).
    virtual void redraw() const = 0;

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutWidget(LayoutItem* parent_layout)
        : LayoutItem(parent_layout)
    {
    }

protected: // fields
    /// \brief The contained Widget.
    std::weak_ptr<Widget> m_widget;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Abstract baseclass for all Layouts.
///
class Layout : public LayoutItem {

    friend class LayoutComponent;

public: // methods
    /// \brief Virtual destructor.
    virtual ~Layout();

    /// \brief Returns the horizontal size range of this LayoutItem.
    virtual const SizeRange& get_horizontal_size() const override = 0;

    /// \brief Returns the vertical size range of this LayoutItem.
    virtual const SizeRange& get_vertical_size() const override = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the ancestry).
    virtual void redraw() const override = 0;

    /// \brief Returns true iff this LayoutItem is hidden.
    virtual bool is_visible() const override { return m_is_visible; }

    /// \brief Shows or hides the Layout.
    /// set_visible(false) hides all items in the Layout, while set_visible(true) only shows those that are not
    /// themselves hidden.
    /// Layouts start out visible.
    void set_visible(const bool is_visible)
    {
        if (is_visible != m_is_visible) {
            m_is_visible = is_visible;
            redraw();
        }
    }

protected: // methods
    /// \brief Value Constructor.
    Layout(Layout* parent)
        : LayoutItem(parent)
        , m_is_visible(true)
    {
    }

private: // fields
    /// \brief Whether to show or hide this Layout.
    bool m_is_visible;
};

} // namespace signal
