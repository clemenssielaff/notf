#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "common/handle.hpp"
#include "core/component.hpp"

namespace signal {

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
/// However, a Widget may not have a LayoutComponent at all, which is why it is not an integral part of the Widget.
///
class LayoutComponent : public Component {

    friend class Layout;

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
    /// \return Pointer to the created Layout. Is empty if the given name is not unique.
    ///
    /// Use as follows:
    ///     std::shared_ptr<MyLayout> = layout_component->create_layout<MyLayout>("unique name");
    /// where MyLayout is a specialization of Layout (otherwise, the template will not compile).
    template <typename LAYOUT, typename = typename std::enable_if<std::is_base_of<Layout, LAYOUT>::value>::type>
    std::shared_ptr<LAYOUT> create_layout(const std::string& name);

    /// \brief Sets the internal layout of this Component.
    /// If the internal layout is already defined, the old one is replaced.
    /// If the given Layout was not created by this Component, the call fails without changing the internal layout.
    /// If the given Layout pointer is empty, the internal layout is removed without being replaced.
    void set_internal_layout(std::shared_ptr<Layout> layout);

protected: // methods
    /// \brief Value Constructor.
    explicit LayoutComponent();

private: // fields
    /// \brief All Layouts of this Layout Component, accessible by name.
    std::unordered_map<std::string, std::shared_ptr<Layout>> m_layouts;

    /// \brief The internal layout.
    std::weak_ptr<Layout> m_internal_layout;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Abstraction layer for something that can be put into a Layout - a Widget or another Layout.
class LayoutItem {

public: // methods
    /// \brief Virtual destructor.
    virtual ~LayoutItem();

    /// \brief Returns the horizontal size range of this LayoutItem.
    virtual const SizeRange& get_horizontal_size() = 0;

    /// \brief Returns the vertical size range of this LayoutItem.
    virtual const SizeRange& get_vertical_size() = 0;

    /// \brief Returns true iff this LayoutItem is visible, false if it ishidden.
    virtual bool is_visible() const = 0;


    // TODO: CONTINUE HERE
    // We need a way for any LayoutItem to update its LayoutComponent, that in turn redraws the Widget
    // Then, we need a LayoutItem subclass wrapping a widget instead of a Layout

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Abstract baseclass for all Layouts.
/// Layouts have a name which must be unique in the LayoutComponent that contains them.
/// It is the programmer's job to ensure that they are.
///
class Layout : public LayoutItem {

    friend class LayoutComponent;

public: // methods
    /// \brief Virtual destructor.
    virtual ~Layout();

    /// \brief Returns the name of this Layout.
    const std::string& get_name() const { return m_name; }

    /// \brief Returns true iff this LayoutItem is hidden.
    virtual bool is_visible() const override { return m_is_visible; }

    /// \brief Shows or hides the Layout.
    /// set_visible(false) hides all items in the Layout, while set_visible(true) only shows those that are not
    /// themselves hidden .
    void set_visible(const bool is_visible)
    {
        if (is_visible != m_is_visible) {
            m_is_visible = is_visible;
            m_layout_component->redraw_widgets();
        }
    }

protected: // methods
    /// \brief Value Constructor.
    Layout(LayoutComponent* layout_component, std::string name)
        : m_layout_component(layout_component)
        , m_name(std::move(name))
    {
    }

private: // methods for LayoutComponent
    /// \brief Returns the LayoutComponent that owns this Layout.
    const LayoutComponent* get_layout_component() const { return m_layout_component; }

private: // fields
    /// \brief LayoutComponent that owns this Layout.
    LayoutComponent* m_layout_component;

    /// \brief Name, is unique in the LayoutComponent owning this Layout.
    const std::string m_name;

    /// \brief Whether to show or hide this Layout.
    bool m_is_visible;
};

} // namespace signal
