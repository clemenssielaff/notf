#include "app/scene.hpp"

#include "app/root_node.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

Scene::scene_name_error::~scene_name_error() = default;

Scene::no_graph_error::~no_graph_error() = default;

Scene::hierarchy_error::~hierarchy_error() = default;

// ================================================================================================================== //

Scene::Scene(const FactoryToken&, const valid_ptr<SceneGraphPtr>& graph, std::string name)
    : m_graph(graph.get())
    , m_name(_validate_scene_name(*graph.get(), std::move(name)))
    , m_root(RootNode::Access<Scene>::create(*this))
{}

Scene::~Scene() = default;

PropertyReader Scene::property(const Path& path) const
{
    if (path.is_empty()) {
        notf_throw_format(Path::path_error, "Cannot query Property from Scene with an empty path");
    }
    if (!path.is_property()) {
        notf_throw_format(Path::path_error, "Path \"{}\" does not identify a Property", path.to_string())
    }
    if (path.is_absolute() && path[0] != name()) {
        notf_throw_format(Path::path_error, "Path \"{}\" does not refer to this Scene (\"{}\")", path.to_string(),
                          name());
    }
    return _property(path);
}

size_t Scene::count_nodes() const
{
    return m_root->count_descendants() + 1; // +1 is the root node
}

void Scene::clear() { m_root->clear(); }

risky_ptr<NodeContainer*> Scene::_get_frozen_children(valid_ptr<const Node*> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph()));

    auto it = m_frozen_children.find(node);
    if (it == m_frozen_children.end()) {
        return nullptr;
    }
    return &it->second;
}

void Scene::_create_frozen_children(valid_ptr<const Node*> node)
{
    NOTF_MUTEX_GUARD(SceneGraph::Access<Scene>::mutex(*graph()));

    auto it = m_frozen_children.emplace(std::make_pair(node, Node::Access<Scene>::children(*node)));
    NOTF_ASSERT(it.second);
}

PropertyReader Scene::_property(const Path& path) const
{
    if (path.is_absolute()) {
        if (path.size() == 1) {
            notf_throw_format(Path::path_error, "Path \"{}\" does not refer to a Property in Scene \"{}\"",
                              path.to_string(), name());
        }
        if (path.size() == 2) {
            // TODO: return SceneProperty
        }
        else {
            // TODO: return NodeProperty from root node
        }
    }
    else { // path.is_relative
        if (path.size() == 1) {
            // TODO: return SceneProperty
        }
        else {
            // TODO: return NodeProperty from root node
        }
    }
}

void Scene::_clear_delta()
{
    // clean all tweaked nodes, regardless if they were deleted or not
    for (const valid_ptr<NodePtr>& tweaked_node : m_tweaked_nodes) {
        Node::Access<Scene>::clean_tweaks(*tweaked_node.get());
    }
    m_tweaked_nodes.clear();

#ifdef NOTF_DEBUG
    // In debug mode, I'd like to ensure that each Node is deleted before its parent.
    // If the parent has been modified (creating a delta referencing all child nodes) and is then deleted, calling
    // `deltas.clear()` might end up deleting the parent node before its childen.
    // Therefore we loop over all deltas and pick out those that can safely be deleted.
    bool made_progress = true;
    while (!m_frozen_children.empty() && made_progress) {
        made_progress = false;
        for (auto it = m_frozen_children.begin(); it != m_frozen_children.end(); ++it) {
            bool childHasDelta = false;
            const NodeContainer& container = it->second;
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

SceneGraph::SceneMap::const_iterator Scene::_validate_scene_name(SceneGraph& graph, std::string name)
{
    auto result = SceneGraph::Access<Scene>::reserve_scene_name(graph, std::move(name));
    if (result.second) {
        return result.first;
    }
    notf_throw_format(scene_name_error,
                      "Cannot create new Scene because its name \"{}\" is not unique within its SceneGraph",
                      result.first->first);
}

NOTF_CLOSE_NAMESPACE
