#include "notf/app/graph/graph.hpp"

#include "notf/app/graph/root_node.hpp"
#include "notf/app/graph/window.hpp"

#include "notf/common/mnemonic.hpp"

NOTF_OPEN_NAMESPACE
namespace detail {

// graph - node registry ============================================================================================ //

AnyNodeHandle Graph::NodeRegistry::get_node(Uuid uuid) const {
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { return iter->second; }
    return {}; // no node found
}

void Graph::NodeRegistry::add(AnyNodeHandle node) {
    // do not check whether this is the UI thread as we need this method during Application construction
    const Uuid& uuid = node.get_uuid(); // this might throw if the handle is expired, do it before locking the mutex
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        const auto [iter, success] = m_registry.try_emplace(uuid, node);
        if (NOTF_UNLIKELY(!success && iter->second != node)) {
            // very unlikely, close to impossible without severe hacking and const-away casting
            NOTF_THROW(NotUniqueError, "A different Node with the UUID {} is already registered with the Graph",
                       uuid.to_string());
        }
    }
}

void Graph::NodeRegistry::remove(const Uuid uuid) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto iter = m_registry.find(uuid); iter != m_registry.end()) { m_registry.erase(iter); }
    m_name_register.remove(uuid);
}

AnyNodeHandle Graph::NodeRegistry::get_node(const std::string& name) const {
    NOTF_GUARD(std::lock_guard(m_mutex));
    if (auto uuid = m_name_register.get(name); uuid.has_value()) {
        if (auto itr = m_registry.find(uuid.value()); itr != m_registry.end()) { return itr->second; }
    }
    return {}; // empty handle
}

std::string Graph::NodeRegistry::get_name(Uuid uuid) {
    {
        NOTF_GUARD(std::lock_guard(m_mutex));
        if (auto name = m_name_register.get(uuid); name.has_value()) { return name.value(); }
    }
    // if the name doesn't exist, make one up
    return set_name(uuid, number_to_mnemonic(hash(uuid), /*max_syllables=*/4));
}

std::string Graph::NodeRegistry::set_name(const Uuid uuid, const std::string& proposal) {
    NOTF_ASSERT(this_thread::is_the_ui_thread());
    NOTF_GUARD(std::lock_guard(m_mutex));

    // if the node already exists under another name, we first have to unregister the old name
    m_name_register.remove(uuid);

    std::string name = proposal;
    { // (re-)register the node under its proposed name, or a unique variant thereof
        size_t counter = 2;
        while (m_name_register.contains(name)) {
            name = fmt::format("{}_{:0>2}", proposal, counter++);
        }
        m_name_register.set(uuid, name);
    }
    return name;
}

// graph ============================================================================================================ //

Graph::Graph() {
    // create the new root node ...
    m_root_node = std::make_shared<RootNode>();
    RootNode::AccessFor<Graph>::finalize(*m_root_node);

    // ... and register it
    AnyNodePtr node = std::static_pointer_cast<AnyNode>(m_root_node);
    m_node_registry.add(node);
    m_dirty_nodes.emplace(std::move(node));
}

Graph::~Graph() {
    m_root_node.reset(); // erase all nodes by erasing the root
}

RootNodeHandle Graph::get_root_node() { return m_root_node; }

std::vector<AnyNodeHandle> Graph::synchronize() {
    NOTF_ASSERT(this_thread::is_the_ui_thread());

    if (m_dirty_nodes.empty()) {
        return {}; // nothing changed
    }

    for (auto& handle : m_dirty_nodes) {
        if (AnyNodePtr node = AnyNodeHandle::AccessFor<Graph>::get_node_ptr(handle)) {
            AnyNode::AccessFor<Graph>::clear_modified_data(*node);
        }
    }
    m_dirty_nodes.clear();

    // TODO: Actually get Windows with dirty Nodes from synchronization
    if (m_root_node->get_child_count() > 0) { return {m_root_node->get_child(0)}; }
    return {};
}

} // namespace detail

NOTF_CLOSE_NAMESPACE
