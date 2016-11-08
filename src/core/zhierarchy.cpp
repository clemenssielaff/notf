#include "core/zhierarchy.hpp"

#include <algorithm>
#include <assert.h>
#include <limits>

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

    // root
    if (m_current == m_root) {
        // right end
        if (m_current->m_right_children.empty()) {
            m_current = nullptr;
        }
        // go to first child on the right
        else {
            m_current = m_current->m_right_children.front();
            dig_left();
        }
    }
    else {
        assert(m_current->m_parent);

        // left from parent
        if (m_current->m_placement == ZNode::left) {

            // go to first child on the right
            if (!m_current->m_right_children.empty()) {
                m_current = m_current->m_right_children.front();
                dig_left();
            }
            else {
                // go to next sibling on the left of parent
                if (m_current->m_index + 1U < m_current->m_parent->m_left_children.size()) {
                    m_current = m_current->m_parent->m_left_children[m_current->m_index + 1];
                    dig_left();
                }
                // go to parent
                else {
                    m_current = m_current->m_parent;
                }
            }
        }
        // currently right from parent
        else {

            // go to first child on the right
            if (!m_current->m_right_children.empty()) {
                m_current = m_current->m_right_children.front();
                dig_left();
            }
            else {
                // go to next sibling on the right of parent
                if (m_current->m_index + 1U < m_current->m_parent->m_right_children.size()) {
                    m_current = m_current->m_parent->m_right_children[m_current->m_index + 1];
                    dig_left();
                }
                // right end
                else {
                    m_current = nullptr;
                }
            }
        }
    }

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
    if (m_parent) {
        // all ZNodes that are a descendant of this (in the Z hierarchy), but whose LayoutItem is not a descendant of
        // this one's LayoutItem (in the LayoutItem hierarchy) need to be moved above this node in the Z hierarchy
        // before this is removed
        std::vector<ZNode*> survivors; // survivors in order from back to front
#ifdef _TEST
        if(m_layout_item){
#endif
            ZIterator it(this);
            while (ZNode* descendant = it.next()) {
                if (!descendant->m_layout_item->has_ancestor(m_layout_item)) {
                    survivors.emplace_back(descendant);
                }
            }
#ifdef _TEST
        }
#endif

        // unparent yourself
        unparent();

        // place the survivors starting at where this node used to be
        if (!survivors.empty()) {
            std::vector<ZNode*>& siblings = m_placement == left ? m_parent->m_left_children : m_parent->m_right_children;
            siblings.insert(siblings.begin() + m_index, survivors.begin(), survivors.end());

            assert(survivors.size() < std::numeric_limits<int>::max());
            m_parent->add_num_descendants(m_placement, static_cast<int>(survivors.size()));
        }

        m_parent = nullptr;
    }

    //  unparent your children
    for (ZNode* child : m_left_children) {
        child->m_parent = nullptr;
    }
    m_left_children.clear();

    for (ZNode* child : m_right_children) {
        child->m_parent = nullptr;
    }
    m_right_children.clear();
}

uint ZNode::getZ() const
{
    assert(m_num_left_descendants >= 0);
    uint result = static_cast<uint>(m_num_left_descendants);
    const ZNode* it = this;
    while (it->m_parent) {
        if (m_placement == left) {
            // visit sibiling on the left
            if (it->m_index > 0) {
                it = it->m_parent->m_left_children[it->m_index - 1];
                result += static_cast<uint>(it->m_num_left_descendants + it->m_num_right_descendants + 1);
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
                result += static_cast<uint>(it->m_num_left_descendants + it->m_num_right_descendants + 1);
            }
            // climb up
            else {
                it = it->m_parent;
                result += static_cast<uint>(it->m_num_left_descendants + 1);
            }
        }
    }
    return result;
}

void ZNode::place_on_top_of(ZNode* parent)
{
    if (m_parent) {
        unparent();
    }

    m_parent = parent;
    m_parent->m_right_children.push_back(this);

    const int descendant_count = m_num_left_descendants + m_num_right_descendants + 1;
    assert(descendant_count > 0); // no wrap
    m_parent->add_num_descendants(right, descendant_count);

    m_placement = right;
    assert(m_parent->m_right_children.size() < std::numeric_limits<ushort>::max());
    m_index = static_cast<ushort>(m_parent->m_right_children.size() - 1);
}

void ZNode::place_on_bottom_of(ZNode* parent)
{
    if (m_parent) {
        unparent();
    }

    m_parent = parent;
    m_parent->m_left_children.emplace(m_parent->m_left_children.begin(), this);

    const int descendant_count = m_num_left_descendants + m_num_right_descendants + 1;
    assert(descendant_count > 0); // no wrap
    m_parent->add_num_descendants(left, descendant_count);

    m_placement = left;
    assert(m_parent->m_left_children.size() < std::numeric_limits<ushort>::max());
    m_parent->update_indices(left, 0);
}

void ZNode::place_in_front_of(ZNode* sibling)
{
    if (!sibling->m_parent) {
        return place_on_top_of(sibling);
    }

    if (m_parent) {
        unparent();
    }

    m_parent = sibling->m_parent;
    m_placement = sibling->m_placement;

    std::vector<ZNode*>& children = m_placement == left ? m_parent->m_left_children : m_parent->m_right_children;
    children.emplace(children.begin() + sibling->m_index + 1, this);
    assert(children.size() < std::numeric_limits<ushort>::max());
    m_parent->update_indices(m_placement, sibling->m_index + 1);

    const int descendant_count = m_num_left_descendants + m_num_right_descendants + 1;
    assert(descendant_count > 0); // no wrap
    m_parent->add_num_descendants(m_placement, descendant_count);
}

void ZNode::place_behind(ZNode* sibling)
{
    if (!sibling->m_parent) {
        return place_on_bottom_of(sibling);
    }

    if (m_parent) {
        unparent();
    }

    m_parent = sibling->m_parent;
    m_placement = sibling->m_placement;

    std::vector<ZNode*>& children = m_placement == left ? m_parent->m_left_children : m_parent->m_right_children;
    children.emplace(children.begin() + sibling->m_index, this);
    assert(children.size() < std::numeric_limits<ushort>::max());
    m_parent->update_indices(m_placement, sibling->m_index);

    const int descendant_count = m_num_left_descendants + m_num_right_descendants + 1;
    assert(descendant_count > 0); // no wrap
    m_parent->add_num_descendants(m_placement, descendant_count);
}

void ZNode::unparent()
{
    if (m_placement == left) {
        m_parent->m_left_children.erase(m_parent->m_left_children.begin() + m_index);
    }
    else {
        m_parent->m_right_children.erase(m_parent->m_right_children.begin() + m_index);
    }

    m_parent->update_indices(m_placement, m_index);

    const int descendant_count = m_num_left_descendants + m_num_right_descendants + 1;
    assert(descendant_count > 0); // no wrap
    m_parent->add_num_descendants(m_placement, -descendant_count);
}

void ZNode::update_indices(PLACEMENT placement, const ushort first_index)
{
    std::vector<ZNode*>& children = placement == left ? m_left_children : m_right_children;
    assert(children.size() < std::numeric_limits<ushort>::max());
    for (ushort index = first_index; index < children.size(); ++index) {
        children[index]->m_index = index;
    }
}

void ZNode::add_num_descendants(PLACEMENT placement, int delta)
{
    int& num_descendants = placement == left ? m_num_left_descendants : m_num_right_descendants;
    num_descendants += delta;
    assert(num_descendants >= 0);
    if (m_parent) {
        m_parent->add_num_descendants(m_placement, delta);
    }
}

} // namespace notf
