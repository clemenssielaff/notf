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
                assert(m_current->m_index < std::numeric_limits<int>::max());
                assert(m_current->m_index >= 0);
                assert(m_current->m_parent->m_left_children.size() <= std::numeric_limits<int>::max());

                // go to next sibling on the left of parent
                if (m_current->m_index + 1 < static_cast<int>(m_current->m_parent->m_left_children.size())) {
                    m_current = m_current->m_parent->m_left_children[static_cast<size_t>(m_current->m_index + 1)];
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
                assert(m_current->m_index < std::numeric_limits<int>::max());
                assert(m_current->m_index >= 0);
                assert(m_current->m_parent->m_right_children.size() <= std::numeric_limits<int>::max());

                // go to next sibling on the right of parent
                if (m_current->m_index + 1 < static_cast<int>(m_current->m_parent->m_right_children.size()))
                {
                    m_current = m_current->m_parent->m_right_children[static_cast<size_t>(m_current->m_index + 1)];
                    dig_left();
                }
                // right end
                else
                {
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
}

ZNode::~ZNode()
{
    if (m_parent) {
        // all ZNodes that are a descendant of this (in the Z hierarchy), but whose LayoutItem is not a descendant of
        // this one's LayoutItem (in the LayoutItem hierarchy) need to be moved above this node in the Z hierarchy
        // before this is removed
        ZIterator it(this);
        std::vector<ZNode*> survivors; // survivors in order from back to front
        while (ZNode* descendant = it.next()) {
            if (!descendant->m_layout_item->has_ancestor(m_layout_item)) {
                survivors.emplace_back(descendant);
            }
        }
        if (!survivors.empty()) {
            // place the survivors just right of this node in the parent
            if (m_placement == left) {
                m_parent->m_left_children.insert(m_parent->m_left_children.begin() + m_index + 1, survivors.begin(), survivors.end());
            }
            else {
                m_parent->m_right_children.insert(m_parent->m_right_children.begin() + m_index + 1, survivors.begin(), survivors.end());
            }
        }

        // unparent yourself
        if (m_placement == left) {
            m_parent->m_left_children.erase(m_parent->m_left_children.begin() + m_index);
        }
        else {
            m_parent->m_right_children.erase(m_parent->m_right_children.begin() + m_index);
        }
        m_parent->update_indices(m_placement, m_index);

        m_parent->add_num_descendants(m_placement, static_cast<int>(survivors.size()) - 1);
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

void ZNode::update_indices(PLACEMENT placement, const int first_index)
{
    std::vector<ZNode*>& children = placement == left ? m_left_children : m_right_children;
    assert(first_index >= 0);
    assert(children.size() <= std::numeric_limits<int>::max());
    for (size_t index = static_cast<size_t>(first_index); index < children.size(); ++index) {
        children[static_cast<size_t>(index)]->m_index = static_cast<int>(index);
    }
}

void ZNode::add_num_descendants(PLACEMENT placement, int delta)
{
    if(delta == 0){
        return;
    }

    unsigned int& num_descendants = placement == left ? m_num_left_descendants : m_num_right_descendants;
    unsigned int old_value = num_descendants;
    if(delta > 0){
        num_descendants += static_cast<unsigned int>(delta);
        assert(num_descendants > old_value);
    } else {
        num_descendants = num_descendants - static_cast<unsigned int>(std::abs(delta));
        assert(num_descendants < old_value);
    }
}

} // namespace notf
