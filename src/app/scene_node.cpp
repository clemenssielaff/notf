#include "app/scene_node.hpp"

#include <atomic>
#include <sstream>
#include <unordered_set>

#include "app/application.hpp"
#include "app/scene_manager.hpp"
#include "common/log.hpp"
#include "common/vector.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Returns the next available SceneNodeId.
/// Is thread-safe and ever-increasing.
std::string next_name()
{
    static std::atomic<size_t> g_nextID(1);
    std::stringstream ss;
    ss << "SceneNode#" << g_nextID++;
    return ss.str();
}

} // namespace

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

namespace detail {

SceneNodeBase::~SceneNodeBase() = default;

} // namespace detail

//====================================================================================================================//

SceneNode::hierarchy_error::~hierarchy_error() = default;

//====================================================================================================================//

SceneNode::SceneNode(const Token&, SceneManager& manager, SceneNodeParent parent)
    : SceneNodeBase(manager), m_parent(parent), m_name(next_name())
{
    std::vector<valid_ptr<SceneNode*>>& siblings = m_parent.get()->m_children;
    siblings.emplace_back(this);
    NOTF_ASSERT(std::count(siblings.begin(), siblings.end(), this) == 1);

    log_trace << "Created SceneNode: \"" << m_name << "\"";
}

SceneNode::~SceneNode()
{
    log_trace << "Destroying SceneNode \"" << m_name << "\"";

    { // unregister from parent
        auto& children = m_parent.get()->m_children;
        const auto it = find(children.begin(), children.end(), this);
        if (it == children.end()) {
            log_critical << "Cannot unregister SceneNode \"" << m_name << "\" from parent";
        }
        else {
            children.erase(it);
        }
    }

    m_children.clear();
}

bool SceneNode::has_ancestor(const SceneNode* ancestor) const
{
    if (!ancestor) {
        return false;
    }

    valid_ptr<const SceneNode*> parent = m_parent.get();
    valid_ptr<const SceneNode*> next = parent->parent();
    while (parent != next) {
        if (parent == ancestor) {
            return true;
        }
        parent = next;
        next = parent->parent();
    }
    return false;
}

risky_ptr<SceneNode*> SceneNode::common_ancestor(SceneNode* other)
{
    if (this == other) {
        return this;
    }

    SceneNode* first = this;
    SceneNode* second = other;

    std::unordered_set<SceneNode*> known_ancestors = {first, second};
    while (1) {
        if (first) {
            first = first->parent();
            if (known_ancestors.count(first)) {
                return first;
            }
            known_ancestors.insert(first);
        }
        if (second) {
            second = second->parent();
            if (known_ancestors.count(second)) {
                return second;
            }
            known_ancestors.insert(second);
        }
    }
}

const std::string& SceneNode::set_name(std::string name)
{
    if (name != m_name) {
        m_name = std::move(name);
//        on_name_changed(m_name);
    }
    return m_name;
}

void SceneNode::stack_front()
{
    std::vector<valid_ptr<SceneNode*>>& siblings = m_parent.get()->m_children;
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    move_to_front(siblings, it);
}

void SceneNode::stack_back()
{
    std::vector<valid_ptr<SceneNode*>>& siblings = m_parent.get()->m_children;
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    move_to_back(siblings, it);
}

void SceneNode::stack_before(valid_ptr<SceneNode*> sibling)
{
    std::vector<valid_ptr<SceneNode*>>& siblings = m_parent.get()->m_children;
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    auto other = std::find(siblings.begin(), siblings.end(), sibling);
    if (other == siblings.end()) {
        notf_throw_format(hierarchy_error, "Cannot stack SceneNode \"" << name() << "\" before SceneNode \""
                                                                       << sibling->name()
                                                                       << "\" that is not a sibling.");
    }
    notf::stack_before(siblings, it, other);
}

void SceneNode::stack_behind(valid_ptr<SceneNode*> sibling)
{
    std::vector<valid_ptr<SceneNode*>>& siblings = m_parent.get()->m_children;
    auto it = std::find(siblings.begin(), siblings.end(), this);
    NOTF_ASSERT(it != siblings.end());
    auto other = std::find(siblings.begin(), siblings.end(), sibling);
    if (other == siblings.end()) {
        notf_throw_format(hierarchy_error, "Cannot stack SceneNode \"" << name() << "\" behind SceneNode \""
                                                                       << sibling->name()
                                                                       << "\" that is not a sibling.");
    }
    notf::stack_behind(siblings, it, other);
}

NOTF_CLOSE_NAMESPACE
