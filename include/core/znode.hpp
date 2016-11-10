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

private:
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
    size_t get_z() const;

    /** Returns the parent node of this ZNode. */
    const ZNode* get_parent() const { return m_parent; }

    /** Moves under `parent`, all the way in the front. */
    void place_on_top_of(ZNode* parent);

    /** Moves under `parent`, all the way in the back. */
    void place_on_bottom_of(ZNode* parent);

    /** Moves under the same parent as `sibling`, one step above `sibiling`.
     * If `sibling` has no parent, it moves this ZNode to be the leftmost right child of `sibling` instead.
     */
    void place_above(ZNode* sibling);

    /** Moves under the same parent as `sibling`, one step below `sibiling`.
     * If `sibling` has no parent, it moves this ZNode to be the rightmost left child of `sibling` instead.
     */
    void place_below(ZNode* sibling);

    /** Returns the flattened hierarchy below this node as a vector. */
    std::vector<ZNode*> flatten_hierarchy() const;

private: // methods
    /** Unparents this ZNode from its current parent. */
    void unparent();

    /** Updates the `m_index` members of children after their vector has been modified. */
    void update_indices(PLACEMENT placement, const size_t first_index);

    /** Called by a child to add descendants.
     * @throw std::runtime_error    If the new number of descendants would exceed the data types's limit.
     */
    void add_num_descendants(PLACEMENT placement, size_t delta);

    /** Called by a child to subtract descendants.
     * @throw std::runtime_error    If the new number of descendants would drop below 0.
     */
    void subtract_num_descendants(PLACEMENT placement, size_t delta);

    /** Checks, if this Node is a descendant of the given (potential) ancestor. */
    bool is_descendant_of(const ZNode* ancestor) const;

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
    size_t m_num_left_descendants;

    /** Total number of all descendants on the right. */
    size_t m_num_right_descendants;

    /** Whether this ZNode is in the left or right children vector of the parent. */
    PLACEMENT m_placement;

    /** Index in the parent left or right children (which one depends on the `placement`). */
    size_t m_index;
};

} // namespace notf
