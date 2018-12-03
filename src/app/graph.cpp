#include "notf/app/graph.hpp"

#include "notf/app/root_node.hpp"

NOTF_OPEN_NAMESPACE
namespace detail {

// graph - freeze guard ============================================================================================= //

Graph::FreezeGuard::FreezeGuard(std::thread::id thread_id) : m_thread_id(std::move(thread_id)) {
    if (!TheGraph()->_freeze(m_thread_id)) { m_thread_id = std::thread::id(); }
}

Graph::FreezeGuard::~FreezeGuard() {
    if (is_valid()) { TheGraph()->_unfreeze(m_thread_id); }
}

// graph - node registry ============================================================================================ //

NodeHandle Graph::NodeRegistry::get_node(Uuid uuid) const {
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { return iter->second; }
    return {}; // no node found
}

void Graph::NodeRegistry::add(NodePtr node) {
    const Uuid& uuid = node->get_uuid();
    NodeHandle handle = std::move(node);
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        const auto [iter, success] = m_registry.try_emplace(uuid, handle);
        if (NOTF_UNLIKELY(!success && iter->second != handle)) {
            // very unlikely, close to impossible without severe hacking and const-away casting
            NOTF_THROW(NotUniqueError, "A different Node with the UUID {} is already registered with the Graph",
                       uuid.to_string());
        }
    }
}

void Graph::NodeRegistry::remove(Uuid uuid) {
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { m_registry.erase(iter); }
}

// the graph - node name registry =================================================================================== //

std::string_view Graph::NodeNameRegistry::set_name(NodeHandle node, const std::string& name) {
    const Uuid& uuid = node->get_uuid(); // this might throw if the handle is expired, do it before locking the mutex
    {
        NOTF_GUARD(std::lock_guard(m_mutex));

        // if the node already exists under another name, we first have to unregister the old name
        if (auto& name_view = m_uuid_to_name[uuid]; !name_view.empty()) {
            _remove_name(name_view);

            auto iter = m_uuid_to_name.find(uuid);
            NOTF_ASSERT(iter != m_uuid_to_name.end());
            m_uuid_to_name.erase(iter);
        }

        std::string_view result;
        { // (re-)register the node under its proposed name, or a unique variant thereof
            size_t counter = 2;
            auto [iter, success] = m_name_to_node.try_emplace(name, std::make_pair(uuid, node));
            while (!success) {
                std::tie(iter, success)
                    = m_name_to_node.try_emplace(fmt::format("{}_{:0>2}", name, counter++), std::make_pair(uuid, node));
            }
            result = iter->first;
        }
        return result;
    }
}

NodeHandle Graph::NodeNameRegistry::get_node(const std::string& name) const {
    NOTF_GUARD(std::lock_guard(m_mutex));

    // find the handle
    auto name_iter = m_name_to_node.find(name);
    if (name_iter == m_name_to_node.end()) { return {}; }

    // if the handle is valid, return it
    NodeHandle& node = name_iter->second.second;
    NOTF_ASSERT(!node.is_expired());
    return node;
}

void Graph::NodeNameRegistry::remove_node(const Uuid uuid) {
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto uuid_iter = m_uuid_to_name.find(uuid); uuid_iter != m_uuid_to_name.end()) {
        _remove_name(uuid_iter->second);
        m_uuid_to_name.erase(uuid_iter);
    }
}

void Graph::NodeNameRegistry::_remove_name(std::string_view name_view) {
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());
    static std::string name; // copy the value of the string_view into a reusable static string
    name = name_view;
    auto iter = m_name_to_node.find(name);
    if (iter != m_name_to_node.end()) { m_name_to_node.erase(iter); }
}

// graph ============================================================================================================ //

Graph::Graph() {
    // create the new root node ...
    m_root_node = std::make_shared<RootNode>();
    RootNode::AccessFor<Graph>::finalize(*m_root_node);

    // ... and register it
    NodePtr node = std::static_pointer_cast<Node>(m_root_node);
    m_node_registry.add(node);
    m_dirty_nodes.emplace(std::move(node));
}

Graph::~Graph() {
    m_root_node.reset(); // erase all nodes by erasing the root
}

RootNodeHandle Graph::get_root_node() { return m_root_node; }

bool Graph::_freeze(const std::thread::id thread_id) {
    if (is_frozen() || thread_id == std::thread::id()) { return false; }
    NOTF_GUARD(std::lock_guard(m_mutex));

    // freeze the graph
    m_freezing_thread = std::move(thread_id);

    return true;
}

void Graph::_unfreeze(const std::thread::id thread_id) {
    if (!is_frozen_by(thread_id)) { return; }
    NOTF_GUARD(std::lock_guard(m_mutex));

    // unfreeze the graph
    m_freezing_thread = std::thread::id();

    _synchronize();
}

bool Graph::_synchronize() {
    NOTF_ASSERT(m_mutex.is_locked_by_this_thread());

    if (m_dirty_nodes.empty()) {
        return false; // nothing changed
    }

    for (auto& handle : m_dirty_nodes) {
        if (NodePtr node = NodeHandle::AccessFor<Graph>::get_node_ptr(handle)) {
            Node::AccessFor<Graph>::clear_modified_data(*node);
        }
    }
    m_dirty_nodes.clear();
    return true; // dirty nodes cleared their modified data
}

} // namespace detail

// helper =========================================================================================================== //

RecursiveMutex& TheGraphMutex() { return TheGraph()->get_graph_mutex(); }

NOTF_CLOSE_NAMESPACE
