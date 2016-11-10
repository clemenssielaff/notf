#include "core/znode.hpp"

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <limits>

#include "common/log.hpp"
#include "core/layout_item.hpp"

namespace notf {

ZIterator::ZIterator(ZNode* root)
    : m_current(root)
    , m_root(root)
{
    // invalid
    if (!m_current) {
        return;
    }

    // start at the very left
    dig_left();
}

ZNode* ZIterator::next()
{
    ZNode* result = m_current;

    // if the iterator has finished, return early
    if (!result) {
        return result;
    }

    // go to first child on the right
    if (!m_current->m_right_children.empty()) {
        m_current = m_current->m_right_children.front();
        dig_left();
        return result;
    }

    while (m_current != m_root) {
        std::vector<ZNode*>& siblings = m_current->m_placement == ZNode::left ? m_current->m_parent->m_left_children
                                                                              : m_current->m_parent->m_right_children;
        // go to next sibling
        if (m_current->m_index + 1 < siblings.size()) {
            m_current = siblings[m_current->m_index + 1];
            dig_left();
            return result;
        }

        // go to parent on the right
        if (m_current->m_placement == ZNode::left) {
            m_current = m_current->m_parent;
            return result;
        }

        // backtrack up the ancestry to the left until you find another node on the right or hit the root again
        m_current = m_current->m_parent;
    }

    // finished traversal
    m_current = nullptr;
    return result;
}

void ZIterator::dig_left()
{
    while (!m_current->m_left_children.empty()) {
        m_current = m_current->m_left_children.front();
    }
}

/**********************************************************************************************************************/

ZNode::ZNode(LayoutItem* layout_item)
    : m_layout_item(layout_item)
    , m_parent(nullptr)
    , m_left_children()
    , m_right_children()
    , m_num_left_descendants(0)
    , m_num_right_descendants(0)
    , m_placement(left)
    , m_index(0)
{
#ifndef _TEST
    assert(m_layout_item); // during testing, ZNodes may not have a LayoutItem attached
#endif
}

ZNode::~ZNode()
{
    std::vector<ZNode*> survivors; // survivors in order from back to front

    if (m_parent) {
        // all ZNodes that are a descendant of this (in the Z hierarchy), but whose LayoutItem is not a descendant of
        // this one's LayoutItem (in the LayoutItem hierarchy) need to be moved above this node in the Z hierarchy
        // before this is removed
        if (m_layout_item) {
            ZIterator it(this);
            while (ZNode* descendant = it.next()) {
                if (descendant != this
                    && descendant->m_layout_item
                    && !descendant->m_layout_item->has_ancestor(m_layout_item)) {
                    survivors.emplace_back(descendant);
                }
            }
        }

        // unparent yourself
        unparent();

        // place the survivors starting at where this node used to be
        if (!survivors.empty()) {
            for(ZNode* survivor : survivors){
                survivor->m_parent = m_parent;
                survivor->m_placement = m_placement;
            }
            std::vector<ZNode*>& siblings = m_placement == left ? m_parent->m_left_children : m_parent->m_right_children;
            auto it = siblings.begin();
            std::advance(it, m_index);
            siblings.insert(it, survivors.begin(), survivors.end());
            m_parent->add_num_descendants(m_placement, survivors.size());
            m_parent->update_indices(m_placement, m_index);
        }

        m_parent = nullptr;
    }

    //  unparent your children
    for (ZNode* child : m_left_children) {
        if (std::find(survivors.begin(), survivors.end(), child) == survivors.end()) {
            child->m_parent = nullptr;
        }
    }
    m_left_children.clear();

    for (ZNode* child : m_right_children) {
        if (std::find(survivors.begin(), survivors.end(), child) == survivors.end()) {
            child->m_parent = nullptr;
        }
    }
    m_right_children.clear();
}

size_t ZNode::get_z() const
{
    size_t result = m_num_left_descendants;
    const ZNode* it = this;
    while (it->m_parent) {
        if (m_placement == left) {
            // visit sibiling on the left
            if (it->m_index > 0) {
                it = it->m_parent->m_left_children[it->m_index - 1];
                result += it->m_num_left_descendants + it->m_num_right_descendants + 1;
            }
            // climb up
            else {
                it = it->m_parent;
            }
        }
        else {
            // visit sibiling on the left
            if (it->m_index > 0) {
                it = it->m_parent->m_right_children[it->m_index - 1];
                result += it->m_num_left_descendants + it->m_num_right_descendants + 1;
            }
            // climb up
            else {
                it = it->m_parent;
                result += it->m_num_left_descendants + 1;
            }
        }
    }
    return result;
}

void ZNode::place_on_top_of(ZNode* parent)
{
    if (parent == this) {
        return;
    }
    else if (parent->is_descendant_of(this)) {
        log_critical << "ZHierarchy cycle detected";
        throw std::runtime_error("ZHierarchy cycle detected");
    }

    // in case of ancestry overflow, this call fails without changing the hierarchy
    parent->add_num_descendants(right, m_num_left_descendants + m_num_right_descendants + 1);

    if (m_parent) {
        unparent();
    }

    m_parent = parent;
    m_placement = right;
    m_parent->m_right_children.push_back(this);
    m_index = m_parent->m_right_children.size() - 1;
}

void ZNode::place_on_bottom_of(ZNode* parent)
{
    if (parent == this) {
        return;
    }
    else if (parent->is_descendant_of(this)) {
        log_critical << "ZHierarchy cycle detected";
        throw std::runtime_error("ZHierarchy cycle detected");
    }

    // in case of ancestry overflow, this call fails without changing the hierarchy
    parent->add_num_descendants(left, m_num_left_descendants + m_num_right_descendants + 1);

    if (m_parent) {
        unparent();
    }

    m_parent = parent;
    m_placement = left;
    m_parent->m_left_children.emplace(m_parent->m_left_children.begin(), this);
    m_parent->update_indices(left, 0);
}

void ZNode::place_above(ZNode* sibling)
{
    if (sibling == this) {
        return;
    }
    else if (sibling->is_descendant_of(this)) {
        log_critical << "ZHierarchy cycle detected";
        throw std::runtime_error("ZHierarchy cycle detected");
    }

    if (!sibling->m_parent) {
        if (sibling->m_right_children.empty()) {
            return place_on_top_of(sibling);
        }
        else {
            return place_below(sibling->m_right_children[0]);
        }
    }

    // in case of ancestry overflow, this call fails without changing the hierarchy
    sibling->m_parent->add_num_descendants(sibling->m_placement, m_num_left_descendants + m_num_right_descendants + 1);

    if (m_parent) {
        unparent();
    }

    m_parent = sibling->m_parent;
    m_placement = sibling->m_placement;

    std::vector<ZNode*>& children = m_placement == left ? m_parent->m_left_children : m_parent->m_right_children;
    auto it = children.begin();
    std::advance(it, sibling->m_index + 1);
    children.emplace(it, this);
    m_parent->update_indices(m_placement, sibling->m_index + 1);
}

void ZNode::place_below(ZNode* sibling)
{
    if (sibling == this) {
        return;
    }
    else if (sibling->is_descendant_of(this)) {
        log_critical << "ZHierarchy cycle detected";
        throw std::runtime_error("ZHierarchy cycle detected");
    }

    if (!sibling->m_parent) {
        if (sibling->m_left_children.empty()) {
            return place_on_bottom_of(sibling);
        }
        else {
            return place_above(sibling->m_left_children.back());
        }
    }

    // in case of ancestry overflow, this call fails without changing the hierarchy
    sibling->m_parent->add_num_descendants(sibling->m_placement, m_num_left_descendants + m_num_right_descendants + 1);

    if (m_parent) {
        unparent();
    }

    m_parent = sibling->m_parent;
    m_placement = sibling->m_placement;

    std::vector<ZNode*>& children = m_placement == left ? m_parent->m_left_children : m_parent->m_right_children;
    auto it = children.begin();
    std::advance(it, sibling->m_index);
    children.emplace(it, this);
    m_parent->update_indices(m_placement, sibling->m_index);
}

std::vector<ZNode*> ZNode::flatten_hierarchy() const
{
    // check for ancestry overflow with all the number of ancestors combined, should be impossible to overflow here
    assert(m_num_left_descendants < std::numeric_limits<decltype(m_num_left_descendants)>::max()
           && std::numeric_limits<decltype(m_num_left_descendants)>::max() - (m_num_left_descendants + 1) > m_num_right_descendants);

    std::vector<ZNode*> result;
    result.reserve(m_num_left_descendants + m_num_right_descendants + 1);
    ZIterator it(const_cast<ZNode*>(this));
    while (ZNode* current = it.next()) {
        result.emplace_back(current);
    }
    return result;
}

void ZNode::unparent()
{
    if (m_placement == left) {
        auto it = m_parent->m_left_children.begin();
        std::advance(it, m_index);
        m_parent->m_left_children.erase(it);
    }
    else {
        auto it = m_parent->m_right_children.begin();
        std::advance(it, m_index);
        m_parent->m_right_children.erase(it);
    }

    m_parent->update_indices(m_placement, m_index);
    m_parent->subtract_num_descendants(m_placement, m_num_left_descendants + m_num_right_descendants + 1);
}

void ZNode::update_indices(PLACEMENT placement, const size_t first_index)
{
    std::vector<ZNode*>& children = placement == left ? m_left_children : m_right_children;
    for (auto index = first_index; index < children.size(); ++index) {
        children[index]->m_index = index;
    }
}

void ZNode::add_num_descendants(PLACEMENT placement, size_t delta)
{
    // To ensure that the final z-value fits into a size_t, each of branch of the ancestry (left and right) may only
    // contain at max: (size_t::max / 2) - 1 children.
    // The -1 at the end is because the node itself counts as well and it is subtracted on both sides because it's
    // easier than just from one (and the 1 missing child won't make a difference).
    auto& num_descendants = placement == left ? m_num_left_descendants : m_num_right_descendants;
    if ((std::numeric_limits<decltype(m_num_left_descendants)>::max() / 2) - 1 - num_descendants < delta) {
        throw std::runtime_error("ZHierarchy ancestry overflow detected");
    }
    num_descendants += delta;
    if (m_parent) {
        m_parent->add_num_descendants(m_placement, delta);
    }
}

void ZNode::subtract_num_descendants(PLACEMENT placement, size_t delta)
{
    auto& num_descendants = placement == left ? m_num_left_descendants : m_num_right_descendants;
    if (delta > num_descendants) {
        throw std::runtime_error("ZHierarchy ancestry underflow detected");
    }
    num_descendants -= delta;
    if (m_parent) {
        m_parent->subtract_num_descendants(m_placement, delta);
    }
}

bool ZNode::is_descendant_of(const ZNode* ancestor) const
{
    const ZNode* it = this;
    while (it) {
        if (ancestor == it) {
            return true;
        }
        it = it->m_parent;
    }
    return false;
}

} // namespace notf
