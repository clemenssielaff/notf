#pragma once

#include "app/scene_node.hpp"

NOTF_OPEN_NAMESPACE
namespace access { // forwards
class _SceneNodeHandle;
} // namespace access

// ===================================================================================================================//

template<class T>
struct SceneNodeHandle {
    static_assert(std::is_base_of<SceneNode, T>::value,
                  "The type wrapped by SceneNodeHandle<T> must be a subclass of SceneNode");

    template<class U, class... Args, typename>
    friend SceneNodeHandle<U> SceneNode::_add_child(Args&&... args);

    friend class access::_SceneNodeHandle;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Access types.
    using Access = access::_SceneNodeHandle;

    // methods ------------------------------------------------------------
private:
    /// @{
    /// Constructor.
    /// @param node     Handled SceneNode.
    /// @throws SceneNode::no_node_error    If the given SceneNode is empty or of the wrong type.
    SceneNodeHandle(std::shared_ptr<T> node) : m_node(node)
    {
        { // test if the given node is of the correct type
            if (!dynamic_cast<T*>(node.get())) {
                notf_throw_format(SceneNode::no_node_error, "Cannot wrap Node \"{}\" into Handler of wrong type",
                                  node->name());
            }
        }
    }
    SceneNodeHandle(std::weak_ptr<T> node) : m_node(std::move(node))
    {

        { // test if the given node is alive and of the correct type
            SceneNodePtr raw_node = m_node.lock();
            if (!raw_node) {
                notf_throw(SceneNode::no_node_error, "Cannot create Handler for empty node");
            }
            if (!dynamic_cast<T*>(raw_node.get())) {
                notf_throw_format(SceneNode::no_node_error, "Cannot wrap Node \"{}\" into Handler of wrong type",
                                  raw_node->name());
            }
        }
    }
    /// @}

public:
    NOTF_NO_COPY_OR_ASSIGN(SceneNodeHandle);

    /// Default constructor.
    SceneNodeHandle() = default;

    /// Move constructor.
    /// @param other    SceneNodeHandle to move from.
    SceneNodeHandle(SceneNodeHandle&& other) : m_node(std::move(other.m_node)) { other.m_node.reset(); }

    /// Move assignment operator.
    /// @param other    SceneNodeHandle to move from.
    SceneNodeHandle& operator=(SceneNodeHandle&& other)
    {
        m_node = std::move(other.m_node);
        other.m_node.reset();
        return *this;
    }

    /// @{
    /// The managed BaseNode instance correctly typed.
    /// @throws SceneNode::no_node_error    If the handled SceneNode has been deleted.
    T* operator->()
    {
        SceneNodePtr raw_node = m_node.lock();
        if (NOTF_UNLIKELY(!raw_node)) {
            notf_throw(SceneNode::no_node_error, "SceneNode has been deleted");
        }
        return static_cast<T*>(raw_node.get());
    }
    const T* operator->() const { return const_cast<SceneNodeHandle<T>*>(this)->operator->(); }
    /// @}

    /// Checks if the Handle is currently valid.
    bool is_valid() const { return !m_node.expired(); }

    // fields -------------------------------------------------------------
private:
    /// Handled SceneNode.
    SceneNodeWeakPtr m_node;
};

// accessors ---------------------------------------------------------------------------------------------------------//

class access::_SceneNodeHandle {
    friend class notf::SceneNode;

    /// Extracts a shared_ptr from a SceneNodeHandle.
    template<class T>
    static risky_ptr<SceneNodePtr> get(const SceneNodeHandle<T>& handle)
    {
        return handle.m_node.lock();
    }
};

NOTF_CLOSE_NAMESPACE
