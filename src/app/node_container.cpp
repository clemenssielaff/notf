#include "app/node_container.hpp"

#include "app/node.hpp"
#include "app/scene.hpp"
#include "common/vector.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

bool NodeContainer::add(valid_ptr<NodePtr> node)
{
    if (contains(node->name())) {
        return false;
    }
    m_names.emplace(std::make_pair(node->name(), node.raw()));
    m_order.emplace_back(std::move(node));
    return true;
}

void NodeContainer::erase(const NodePtr& node)
{
    auto it = std::find(m_order.begin(), m_order.end(), node);
    if (it != m_order.end()) {
        m_names.erase((*it)->name());
        m_order.erase(it);
    }
}

void NodeContainer::stack_front(valid_ptr<const Node*> node)
{
    auto it
        = std::find_if(m_order.begin(), m_order.end(), [&](const auto& sibling) -> bool { return sibling == node; });
    NOTF_ASSERT(it != m_order.end());
    move_to_back(m_order, it); // "in front" means at the end of the vector ordered back to front
}

void NodeContainer::stack_back(valid_ptr<const Node*> node)
{
    auto it
        = std::find_if(m_order.begin(), m_order.end(), [&](const auto& sibling) -> bool { return sibling == node; });
    NOTF_ASSERT(it != m_order.end());
    move_to_front(m_order, it); // "in back" means at the start of the vector ordered back to front
}

void NodeContainer::stack_before(const size_t index, valid_ptr<const Node*> sibling)
{
    auto node_it = iterator_at(m_order, index);
    auto sibling_it = std::find(m_order.begin(), m_order.end(), sibling);
    if (sibling_it == m_order.end()) {
        notf_throw(Scene::hierarchy_error,
                   "Cannot stack node \"{}\" before node \"{}\" because the two are not siblings.", (*node_it)->name(),
                   sibling->name());
    }
    notf::move_behind_of(m_order, node_it, sibling_it);
}

void NodeContainer::stack_behind(const size_t index, valid_ptr<const Node*> sibling)
{
    auto node_it = iterator_at(m_order, index);
    auto sibling_it = std::find(m_order.begin(), m_order.end(), sibling);
    if (sibling_it == m_order.end()) {
        notf_throw(Scene::hierarchy_error,
                   "Cannot stack node \"{}\" before node \"{}\" because the two are not siblings.", (*node_it)->name(),
                   sibling->name());
    }
    notf::move_in_front_of(m_order, node_it, sibling_it);
}

void NodeContainer::reverse() { std::reverse(m_order.begin(), m_order.end()); }

void NodeContainer::_rename(valid_ptr<const Node*> node, std::string new_name)
{
    auto name_it = m_names.find(node->name());
    NOTF_ASSERT(name_it != m_names.end());
    NodeWeakPtr node_ptr = std::move(name_it->second);
    m_names.erase(name_it);
    m_names.emplace(std::make_pair(std::move(new_name), std::move(node_ptr)));
}

NOTF_CLOSE_NAMESPACE
