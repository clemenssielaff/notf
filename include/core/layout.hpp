#pragma once

#include "core/screen_item.hpp"

namespace notf {

/**********************************************************************************************************************/

/**
 *
 *
 * Explicit and Implicit Claims
 * ----------------------------
 * Claims can either be `explicit` or `implicit`.
 * An implicit Claim is one that is created by combining multiple child Claims into one and is used only by Layouts.
 * Widgets always have an `explicit` Claim, meaning that the various Claim values were supplied by the user and must not
 * be changed by the layouting process.
 * Layouts can also have an explicit Claim if you want them to ignore their child Claims and provide their own instead.
 */
class Layout : public ScreenItem {
    friend class ScreenItem; // can call _update_claim() and _relayout()

protected: // constructor  ********************************************************************************************/
    Layout(ItemContainerPtr container);

public: // methods ****************************************************************************************************/
    /** Set an explicit Claim for this Layout.
     * Layouts with an explicit Claim do not dynamically aggregate one from their children.
     * @param claim     New explicit Claim.
     * @return          True iff the Claim was modified.
     */
    bool set_claim(const Claim claim);

    /** Unsets an explicit Claim and causes the Layout to aggreate its Claim from its children instead.
     * @return          True iff the Claim was modified.
     */
    bool unset_claim();

    /** Whether or not this Layout has an explicit Claim or not.
     * Layouts with an implicit Claim recalculate theirs from their children - those with an explict Claim don't.
     */
    bool has_explicit_claim() const { return m_has_explicit_claim; }

    /** Removes all Items from the Layout. */
    void clear();

    /** The bounding rect of all child ScreenItems. */
    const Aabrf& get_child_aabr() const { return m_child_aabr; }

public: // signals  ***************************************************************************************************/
    /** Emitted when a new child Item was added to this one.
     * @param ItemID of the new child.
     */
    Signal<const Item*> on_child_added;

    /** Emitted when a child Item of this one was removed.
     * @param ItemID of the removed child.
     */
    Signal<const Item*> on_child_removed;

protected: // methods *************************************************************************************************/
    /** Updates the Claim of this Layout.
     * @return  True, iff the Claim was modified.
     */
    bool _update_claim();

    /** Tells this Layout to create a new Claim for itself from the combined Claims of all of its children.
     * @returns The consolidated Claim.
     */
    virtual Claim _consolidate_claim() = 0;

protected: // members *************************************************************************************************/
    /** If true, this Layout provides its own Claim and does not aggregate it from its children. */
    bool m_has_explicit_claim;

    /** The bounding rect of all child ScreenItems.
     * Depending on the Layout, the child aabr can be equal to the Layout's own aabr but doesn't have to be.
     */
    Aabrf m_child_aabr;
};

} // namespace notf
