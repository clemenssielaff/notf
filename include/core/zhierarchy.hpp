#pragma once

#include <stddef.h>
#include <vector>

namespace notf {

using ushort = unsigned short;
using uint = unsigned int;

class LayoutItem;
class ZNode;

/**********************************************************************************************************************/

/** Iterator over all ZNode%s in order from back to front. */
class ZIterator {

public: // methods
    /** The `root` agrument is a ZNode acting as the root of the traversal.
     * @param root     Root node of the traversal, is also traversed.
     */
    ZIterator(ZNode* root);

    /** Advances the Iterator one step.
     * @return  The next ZNode in the traversal or nullptr when it finished.
     */
    ZNode* next();

private: // methods
    /** "Digs" down to the furtherst child on the left from the current one. */
    void dig_left();

private: // fields
    /** Current ZNode being traversed, is returned by next() and advanced to the next one.
     * Is nullptr when the iteration finished.
     */
    ZNode* m_current;

    /** Root of the iteration. */
    const ZNode* m_root;
};

/**********************************************************************************************************************/

/** A ZNode is a node in the implicit Z-hierarchy of LayoutItems.
 * Every LayoutItem owns a ZNode and is referred back from it.
 */
class ZNode final {

    friend class ZIterator;

    /** Denotes how the ZNode is related to its parent.
     */
    enum PLACEMENT : char {
        left,
        right,
    };

public: // methods
    /** @param layout_item  The LayoutItem owning this ZNode, must outlive this instance. */
    explicit ZNode(LayoutItem* layout_item);

    /** Disconnects this ZNode before destruction.
     * It is more efficient to delete the ZNode highest in the hierarchy first.
     */
    ~ZNode();

    /** Calculates and returns the Z value of this ZNode. */
    uint getZ() const;

    /** Moves this ZNode to the top of the stack of the given parent node. */
    void place_on_top_of(ZNode* parent);

    /** Moves this ZNode to the bottom of the stack of the given parent node. */
    void place_on_bottom_of(ZNode* parent);

    /** Moves this ZNode under the same parent as `sibling`, one step in front of `sibiling`. */
    void place_in_front_of(ZNode* sibling);

    /** Moves this ZNode under the same parent as `sibling`, one step behind of `sibiling`. */
    void place_behind(ZNode* sibling);

private: // methods
    /** Unparents this ZNode from its current parent. */
    void unparent();

    /** Updates the `m_index` members of children after their vector has been modified. */
    void update_indices(PLACEMENT placement, const ushort first_index);

    /** Called by a child to update the number of descendants. */
    void add_num_descendants(PLACEMENT placement, int delta);

private: // fields
    /** The LayoutItem owning this ZNode. */
    LayoutItem* const m_layout_item;

    /** The parent of this ZNode, is nullptr if this is the root. */
    ZNode* m_parent;

    /** Children on the left of this ZNode. */
    std::vector<ZNode*> m_left_children;

    /** Children on the right of this ZNode. */
    std::vector<ZNode*> m_right_children;

    /** Total number of all descendants on the left. */
    int m_num_left_descendants;

    /** Total number of all descendants on the right. */
    int m_num_right_descendants;

    /** Whether this ZNode is in the left or right children vector of the parent. */
    PLACEMENT m_placement;

    /** Index in the parent left or right children (which one depends on the `placement`). */
    ushort m_index;
    // TODO: document the assumption that each LayoutItem and each ZNode only has maximal std::numeric_limits<short>::max() children
};

} // namespace notf
