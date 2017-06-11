#pragma once

#include "core/screen_item.hpp"

namespace notf {

/**********************************************************************************************************************/

/**
 *
 *
 */
class Layout : public ScreenItem {
    friend class ScreenItem; // can call _update_claim() and _relayout()

protected: // constructor  ********************************************************************************************/
    Layout(ItemContainerPtr container);

public: // methods ****************************************************************************************************/
    /** (Un-)Sets an explicit Claim for this Layout.
     * Layouts with an explicit Claim do not dynamically aggregate one from their children.
     * To unset an explicit Claim, pass a zero Claim.
     * @param claim New explicit Claim.
     * @return True iff the Claim was modified.
     */
    bool set_claim(const Claim claim);

    /** Removes all Items from the Layout. */
    void clear();

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
};

} // namespace notf
