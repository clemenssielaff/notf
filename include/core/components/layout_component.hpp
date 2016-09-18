#pragma once

/*
 * How to layout
 *
 * Layouts are a PART OF THE WIDGET.
 * Layouts should not be a Component, they are too integral to the workings of a widget to be a mere component.
 * Also, there is no real advantage of having it be a Component because it cannot be shared by multiple widgets
 * and is an inherit participant in the widget hierarchy, meaning that all of the Widget's children need to be
 * considered by the Layout Component and all of the Layout Component's Widgets need to be children of the owning
 * Widget.
 *
 * Each Widget has a ROOT LAYOUT
 * The root layout contains a single root layout item
 * Let's make the Root Layout a special class that only the Widget itself can construct.
 *
 * Each LAYOUT ITEM has a single parent layout in which it is contained.
 * A LayoutItem has 1 internal layout and 0-n external ones.
 * The internal layout is use to control the size of the Layout Item from within.
 * Internal layouts can also be other layouts, the root layouts of Widgets for example.
 *
 * A LAYOUT has no concept of a parent, just like a Widget has no concept of what LayoutItem it is in.
 * The parent widget however has a map, mapping each child Widget to a LayoutItem, which is how you can still get from
 * a Widget to its containing LayoutItem.
 * Everything that makes up how a widget behaves in a layout should be part of the Shape Component, we cannot store
 * anything in the LayoutItem because there might be different LayoutItem classes for different layouts.
 * A widget should also contain a second map, from all Layouts to LayoutItems.
 *
 * Layouts have to be created in-place, as child of the parent Layout in which they are nested.
 * They cannot be moved into another layout, but their LayoutItems may be reshuffled or whatever you do in that
 * respective layout.
 * They live in shared pointers are are kept alive by their owning LayoutItem.
 * As you move Widgets in and out of the Layout, the Layout keeps track of how many children it has.
 * If it finds itself with zero children, it asks whether it's layout item holds the only remaining shared pointer.
 * If so, it is removed.
 * If not, the user has stored another shared pointer somewhere and it is now his or her responsibility to clean it up.
 * ... sounds like the LayoutItem should store a weak_ptr, though...
 * Also, a LayoutItem must not keep a reference to a widget that was reparented.
 * Sounds like much of a similar problem...
 *
 */


#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "common/handle.hpp"
#include "core/component.hpp"
#include "utils/smart_enabler.hpp"

namespace signal {

class InternalLayout;
class Layout;
class LayoutItem;
class SizeRange;
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
    /// \brief This Component's type.
    virtual KIND get_kind() const override { return KIND::LAYOUT; }

    /// \brief Returns the LayoutItem at a given local position.
    /// \param local_pos    Local coordinates where to look for the LayoutItem.
    /// \return LayoutItem at the given position or an empty shared pointer, if there is none.
    virtual std::shared_ptr<Widget> widget_at(const Vector2& local_pos) = 0;

    /// \brief Removes a given Widget from this Layout.
    virtual void remove_widget(std::shared_ptr<Widget> widget) = 0;

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

/// \brief Abstraction layer for something that can be put into a Layout - a Widget or another Layout.
class LayoutItem {

public: // methods
    /// \brief Virtual destructor.
    virtual ~LayoutItem();

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
    explicit LayoutItem(LayoutItem* parent_layout)
        : m_parent_layout(parent_layout)
    {
    }

protected: // fields
    /// \brief Layout containing this Layout Item.
    const LayoutItem* m_parent_layout;
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
