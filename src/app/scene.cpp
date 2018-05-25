#include "app/scene.hpp"

#include <algorithm>

#include "app/scene_node.hpp"
#include "common/vector.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

Scene::no_graph_error::~no_graph_error() = default;

Scene::hierarchy_error::~hierarchy_error() = default;

//====================================================================================================================//

bool Scene::NodeContainer::add(valid_ptr<SceneNodePtr> node)
{
    if (contains(node->name())) {
        return false;
    }
    m_names.emplace(std::make_pair(node->name(), node.raw()));
    m_order.emplace_back(std::move(node));
    return true;
}

void Scene::NodeContainer::erase(const SceneNodePtr& node)
{
    auto it = std::find(m_order.begin(), m_order.end(), node);
    if (it != m_order.end()) {
        m_names.erase((*it)->name());
        m_order.erase(it);
    }
}

void Scene::NodeContainer::stack_front(const valid_ptr<SceneNode*> node)
{
    auto it
        = std::find_if(m_order.begin(), m_order.end(), [&](const auto& sibling) -> bool { return sibling == node; });
    NOTF_ASSERT(it != m_order.end());
    move_to_back(m_order, it); // "in front" means at the end of the vector ordered back to front
}

void Scene::NodeContainer::stack_back(const valid_ptr<SceneNode*> node)
{
    auto it
        = std::find_if(m_order.begin(), m_order.end(), [&](const auto& sibling) -> bool { return sibling == node; });
    NOTF_ASSERT(it != m_order.end());
    move_to_front(m_order, it); // "in back" means at the start of the vector ordered back to front
}

void Scene::NodeContainer::stack_before(const size_t index, const valid_ptr<SceneNode*> sibling)
{
    auto node_it = iterator_at(m_order, index);
    auto sibling_it = std::find(m_order.begin(), m_order.end(), sibling);
    if (sibling_it == m_order.end()) {
        notf_throw_format(hierarchy_error,
                          "Cannot stack node \"{}\" before node \"{}\" because the two are not siblings.",
                          (*node_it)->name(), sibling->name());
    }
    notf::move_behind_of(m_order, node_it, sibling_it);
}

void Scene::NodeContainer::stack_behind(const size_t index, const valid_ptr<SceneNode*> sibling)
{
    auto node_it = iterator_at(m_order, index);
    auto sibling_it = std::find(m_order.begin(), m_order.end(), sibling);
    if (sibling_it == m_order.end()) {
        notf_throw_format(hierarchy_error,
                          "Cannot stack node \"{}\" before node \"{}\" because the two are not siblings.",
                          (*node_it)->name(), sibling->name());
    }
    notf::move_in_front_of(m_order, node_it, sibling_it);
}

//====================================================================================================================//

Scene::Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph)
    : m_graph(graph.get()), m_root(std::make_shared<RootSceneNode>(SceneNode::Access<Scene>::create_token(), *this))
{}

Scene::~Scene() = default;

size_t Scene::count_nodes() const
{
    return m_root->count_descendants() + 1; // +1 is the root node
}

void Scene::clear() { m_root->clear(); }

risky_ptr<Scene::NodeContainer*> Scene::_get_frozen_children(valid_ptr<const SceneNode*> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph()));

    auto it = m_frozen_children.find(node);
    if (it == m_frozen_children.end()) {
        return nullptr;
    }
    return &it->second;
}

void Scene::_create_frozen_children(valid_ptr<const SceneNode*> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph()));

    auto it = m_frozen_children.emplace(std::make_pair(node, SceneNode::Access<Scene>::children(*node)));
    NOTF_ASSERT(it.second);
}

void Scene::_clear_delta()
{
    // clean all tweaked nodes, regardless if they were deleted or not
    for (const valid_ptr<SceneNodePtr>& tweaked_node : m_tweaked_nodes) {
        SceneNode::Access<Scene>::clean_tweaks(*tweaked_node.get());
    }
    m_tweaked_nodes.clear();

#ifdef NOTF_DEBUG
    // In debug mode, I'd like to ensure that each SceneNode is deleted before its parent.
    // If the parent has been modified (creating a delta referencing all child nodes) and is then deleted, calling
    // `deltas.clear()` might end up deleting the parent node before its childen.
    // Therefore we loop over all deltas and pick out those that can safely be deleted.
    bool made_progress = true;
    while (!m_frozen_children.empty() && made_progress) {
        made_progress = false;
        for (auto it = m_frozen_children.begin(); it != m_frozen_children.end(); ++it) {
            bool childHasDelta = false;
            const Scene::NodeContainer& container = it->second;
            for (auto& child : container) {
                if (m_frozen_children.count(child.raw().get())) {
                    childHasDelta = true;
                    break;
                }
            }
            if (!childHasDelta) {
                m_frozen_children.erase(it);
                made_progress = true;
                break;
            }
        }
    }
    // If the loop should be broken because we made no progress, a child delta references a parent...
    // Should that ever happen, something has gone seriously wrong.
    NOTF_ASSERT(made_progress);
#else
    m_deltas.clear();
#endif
}

NOTF_CLOSE_NAMESPACE
