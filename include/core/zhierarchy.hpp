#pragma once

#include <algorithm>
#include <assert.h>
#include <memory>
#include <vector>

namespace notf {

class LayoutItem;
using uint = unsigned int;

struct ZNode final {
    ZNode* parent;
    std::vector<ZNode*> left_children;
    std::vector<ZNode*> right_children;
    LayoutItem* const layout_item;

    /** Disconnects this ZNode before destruction.
     * Try to destroy the root of the hierarchy first. This way the children can be unparented from their parent and we
     * save a lot of time removing children from their parent.
     */
    ~ZNode()
    {
        // unparent yourself
        if (parent) {
            auto it = std::find(std::begin(parent->left_children), std::end(parent->left_children), this);
            if (it == std::end(parent->left_children)) {
                it = std::find(std::begin(parent->right_children), std::end(parent->right_children), this);
                if (it == std::end(parent->left_children)) {
                    assert(0);
                }
                else {
                    parent->right_children.erase(it);
                }
            }
            else {
                parent->left_children.erase(it);
            }
        }

        //  unparent your children
        for (ZNode* child : left_children) {
            child->parent = nullptr;
        }
        for (ZNode* child : right_children) {
            child->parent = nullptr;
        }
    }
};

// TODO: continue here
// All layoutItems should have a std::unique_ptr<ZNode>

/** Iterator over all ZNode%s in order from back to front. */
class ZIterator {

private: // types
    enum DIRECTION {
        left,
        center,
        right,
    };

    struct Index {
        DIRECTION direction;
        uint index;

        Index(DIRECTION direction, uint index)
            : direction(direction)
            , index(index)
        {}
    };

public: // methods
    /** The `root` agrument is a ZNode acting as the root of the traversal.
     * @param root     Root node of the traversal, is also traversed.
     */
    ZIterator(ZNode* root)
        : m_current(root)
        , m_indices{{center, 0}}
    {
        // invalid
        if (!m_current) {
            return;
        }

        // start at the very left
        dig_left();
    }

    /** Advances the Iterator one step.
     * @return  The next ZNode in the traversal or nullptr when it finished.
     */
    ZNode* next()
    {
        ZNode* result = m_current;

        // if the iterator has finished, return early
        if (!result) {
            return result;
        }

        assert(!m_indices.empty());
        Index& current_index = m_indices.back();

        // root
        if (current_index.direction == center) {
            if (m_current->right_children.empty()) {
                finish();
            }
            else {
                step_right();
            }
        }
        else {
            assert(m_current->parent);

            // left from parent
            if (current_index.direction == left) {

                // go to first child on the right
                if (!m_current->right_children.empty()) {
                    step_right();
                }
                else {
                    // go to next sibling on the left of parent
                    if (++current_index.index < m_current->parent->left_children.size()) {
                        m_current = m_current->parent->left_children[current_index.index];
                        dig_left();
                    }
                    // go to parent
                    else {
                        m_indices.pop_back();
                        m_current = m_current->parent;
                    }
                }
            }
            // currently right from parent
            else {

                // go to first child on the right
                if (!m_current->right_children.empty()) {
                    step_right();
                }
                else {
                    // go to next sibling on the right of parent
                    if (++current_index.index < m_current->parent->right_children.size()) {
                        m_current = m_current->parent->right_children[current_index.index];
                        dig_left();
                    }
                    // right end
                    else {
                        finish();
                    }
                }
            }
        }

        return result;
    }

private: // methods
    void step_right()
    {
        assert(m_current);
        assert(!m_current->right_children.empty());
        m_indices.emplace_back(right, 0);
        m_current = m_current->right_children.front();
        dig_left();
    }

    void dig_left()
    {
        assert(m_current);
        while (!m_current->left_children.empty()) {
            m_indices.emplace_back(left, 0);
            m_current = m_current->left_children.front();
        }
    }

    void finish()
    {
        m_current = nullptr;
        m_indices.clear();
    }

private: // fields
    ZNode* m_current;

    std::vector<Index> m_indices;
};

} // namespace notf
