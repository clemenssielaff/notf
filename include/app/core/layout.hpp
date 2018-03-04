#pragma once

#include "app/core/screen_item.hpp"

namespace notf {

//====================================================================================================================//

/// Relayout
/// ========
///
/// All Layouts must implement the pure virtual method `_relayout`.
/// It is the main function of a Layout and has multiple responsibilities:
///
/// 1. All visible child ScreenItems must be placed in accordance to the rules of the Layout.
/// 2. The Layout must determine its own Aabr as well as
/// 3. ... its content Aabr.
///
/// Explicit and Implicit Claims
/// ============================
///
/// Claims in a Layout can either be `explicit` or `implicit`.
/// An implicit Claim is one that is created by combining multiple child Claims into one. Layouts can also have an
/// explicit Claim if you want them to ignore their child Claims and provide their own instead.
class Layout : public ScreenItem {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(ScreenItem)

    // signals -------------------------------------------------------------------------------------------------------//
public:
    /// Emitted when a new child Item was added to this one.
    /// @param ItemID of the new child.
    Signal<const Item*> on_child_added;

    /// Emitted when a child Item of this one was removed.
    /// @param ItemID of the removed child.
    Signal<const Item*> on_child_removed;

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param container    Child container.
    Layout(detail::ItemContainerPtr container);

public:
    /// Destructor.
    virtual ~Layout() override;

    /// Set an explicit Claim for this Layout.
    /// Layouts with an explicit Claim do not dynamically aggregate one from their children.
    /// @param claim     New explicit Claim.
    /// @return          True iff the Claim was modified.
    bool set_claim(const Claim claim);

    /// Unsets an explicit Claim and causes the Layout to aggreate its Claim from its children instead.
    /// @return          True iff the Claim was modified.
    bool unset_claim();

    /// Whether or not this Layout has an explicit Claim or not.
    /// Layouts with an implicit Claim recalculate theirs from their children - those with an explict Claim don't.
    bool has_explicit_claim() const { return m_has_explicit_claim; }

    /// Removes all Items from the Layout.
    void clear();

protected:
    /// Updates the Claim of this Layout.
    /// @return  True, iff the Claim was modified.
    bool _update_claim();

    /// Tells this Layout to create a new Claim for itself from the combined Claims of all of its children.
    /// @returns The consolidated Claim.
    virtual Claim _consolidate_claim() = 0;

    // fields --------------------------------------------------------------------------------------------------------//
protected:
    /// If true, this Layout provides its own Claim and does not aggregate it from its children.
    bool m_has_explicit_claim;
};

//====================================================================================================================//

template<>
class Layout::Access<ScreenItem> {
    friend class ScreenItem;

    /// Constructor.
    /// @param layout   Layout to access.
    Access(Layout& layout) : m_layout(layout) {}

    /// Updates the Claim of this Layout.
    /// @return  True, iff the Claim was modified.
    bool update_claim() { return m_layout._update_claim(); }

    /// The ScreenItem to access.
    Layout& m_layout;
};
} // namespace notf
