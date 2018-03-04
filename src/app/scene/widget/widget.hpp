#pragma once

#include "app/scene/widget/capability.hpp"
#include "app/scene/widget/screen_item.hpp"

namespace notf {

//====================================================================================================================//

/// A Widget is something drawn on screen that the user can interact with.
/// The term "Widget" is a mixture of "Window" and "Gadget".
///
/// Capabilities
/// ============
/// Sometimes Layouts need more information from a Widget than just its bounding rect in order to place it correctly.
/// For example, a TextLayout will try to align two subsequent widgets displaying text in a way that makes it look, like
/// both Widgets are part of the same continuous text.
/// This only works, if the TextLayout knows the font size and vertical baseline offset of each of the Widgets.
/// However, these are not fields that are available in the Widget baseclass -- nor should they be, since most other
/// Widgets do not display text in that way.
/// This information is therefore separate from the actual Widget, contained in a so-called Widget "Capability".
/// Any Widget that is capable of being displayed inline in a continuous text will have a certain Capability which can
/// be queried by the TextLayout and used to position the Widget correctly.
class Widget : public ScreenItem {

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    Widget();

public:
    /// Destructor.
    virtual ~Widget() override {}

    /// Returns a requested capability by type.
    /// If the map does not contain the requested capability, throws an std::out_of_range exception.
    template<typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<CAPABILITY> capability()
    {
        return m_capabilities.get<CAPABILITY>();
    }

    template<typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<const CAPABILITY> capability() const
    {
        return m_capabilities.get<CAPABILITY>();
    }

    /// Inserts or replaces a capability of this widget.
    /// @param capability    Capability to insert.
    template<typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    void set_capability(std::shared_ptr<CAPABILITY> capability)
    {
        m_capabilities.set(std::move(capability));
    }

    /// Sets a new Claim for this Widget.
    /// @return True iff the Claim was modified.
    bool set_claim(Claim claim) { return _set_claim(std::move(claim)); }

    /// Tells the SceneManager that this Widget needs to be redrawn.
    void redraw() const;

protected:
    virtual void _relayout() override;

private:
    /// Redraws the Cell with the Widget's current state.
    //    virtual void _paint(Painter& painter) const = 0;

    virtual void _remove_child(const Item*) override { assert(0); } // should never happen

    virtual void _widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const override;

    // hide Item methods that have no effect for Widgets
    using Item::has_child;
    using Item::has_children;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Widget capabilities.
    CapabilityMap m_capabilities;
};

} // namespace notf
