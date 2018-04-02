#pragma once

#include "app/scene_manager.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// The virtual base class for all nodes in a Scene graph.
///
/// Scene Hierarchy
/// ===============
///
/// SceneNodes form a hierarchy with a single RootNode on top. Each node can have 0-n children, although different
/// subtypes may pose additional restrictions.
///
/// Only SceneNodes may create other SceneNodes. The only method allowed to create SceneNodes is in the
/// `SceneManager::Access<SceneNode>` accessor called `add_child`, returning a `ChildSceneNode<T>` (with T being the
/// type of the child node). ChildSceneNodes owns their actual child SceneNode, meaning that when they are destroyed, so
/// is their child SceneNode. You store them in the parent SceneNode, in any way that makes most sense for the type of
/// node (list, map, quadtree...). They are not copy- or assignable, think of them as specialized unique_ptr.
///
/// To traverse the hierarchy, we need to know all of the children of a SceneNode and their draw order, from back
/// to front (earlier nodes are drawn behind later nodes). Therefore, a Node must keep a vector of raw SceneNode
/// pointers that point to the nodes that the node itself owns through NodePtr objects. By default, `add_child` appends
/// a raw pointer to the newly created SceneNode to the `m_children` vector, resulting in each new child being drawn in
/// front of its next-older sibling. You are free to reshuffle the children as you please using the dedicated functions
/// on SceneNode. The destructor of ChildSceneNode automatically finds and removes its SceneNode from the parent's
/// `m_children` vector.
///
/// SceneNodes also have a link back to their parent, in a ParentSceneNode object. They work much the same way as a
/// ChildSceneNode, but do not own the parent (obviously). As parents are guaranteed to outlive their children in the
/// tree, we can be certain that all nodes have a valid parent.
/// The root SceneItem is its own parent, so while it is in fact valid you need to check for this condition when
/// traversing up the hierarchy, otherwise you'll be stuck in an infinite loop (then again, if you don't check for the
/// stop condition and the parent is null, your program crashes).
///
/// Name
/// ====
///
/// Each SceneNode has a name. Be default, the name is of the format "SceneNode#[number]" where the number is guaranteed
/// to be unique. The user can change the name of each SceneNode, in which case it might no longer be unique.
///
/// Signals
/// =======
///
/// SceneNodes communicate with each other either through their relationship in the hierachy (parents to children and
/// vice-versa) or via Signals. Signals have the advantage of being able to connect every SceneNode regardless of its
/// position in the hierarchy, given that both the source and the target are managed by the same SceneManager (for
/// threading reasons). They can be created by the user and enabled/disabled at will. In order to facilitate Signal
/// handling at the lowest possible level, all SceneNodes derive from the `receive_signals` class that takes care of
/// removing leftover connections that still exist once the SceneNode goes out of scope.
///
class SceneNode : public detail::SceneNodeBase {

    // types ---------------------------------------------------------------------------------------------------------//
private:
    using SceneNodeBase = detail::SceneNodeBase;

    /// Thrown when a node did not have the expected position in the hierarchy.
    NOTF_EXCEPTION_TYPE(hierarchy_error)

public:
    /// Convenience container for easy mutable iteration over all of a SceneNode's children.
    struct Children {
        /// Constructor.
        /// @param node Node whose children to iterate over.
        Children(SceneNode& node) : m_node(node) {}

        /// Iterator to the first child.
        auto begin() { return m_node.m_children.begin(); }

        /// Reverse Iterator to the last child.
        auto rbegin() { return m_node.m_children.rbegin(); }

        /// Iterator one past the last child.
        auto end() { return m_node.m_children.end(); }

        /// Iterator one past the last child.
        auto rend() { return m_node.m_children.rend(); }

    private: // fields
        SceneNode& m_node;
    };

    /// Convenience container for easy constant iteration over all of a SceneNode's children.
    struct ConstChildren {
        /// Constructor.
        /// @param node Node whose children to iterate over.
        ConstChildren(const SceneNode& node) : m_node(node) {}

        /// Iterator to the first child.
        auto begin() const { return m_node.m_children.cbegin(); }

        /// Reverse Iterator to the last child.
        auto rbegin() const { return m_node.m_children.crbegin(); }

        /// Iterator one past the last child.
        auto end() const { return m_node.m_children.cend(); }

        /// Iterator one past the last child.
        auto rend() const { return m_node.m_children.crend(); }

    private: // fields
        const SceneNode& m_node;
    };

    //=========================================================================
protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class SceneNode;
        Token() {} // not "= default", otherwise you could construct a Token via `Token{}`.
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    /// @param token    Factory token provided by SceneNode::_add_child.
    /// @param manager  SceneManager containing the Scene of this SceneNode.
    /// @param parent   Parent SceneNode.
    SceneNode(const Token&, SceneManager& manager, SceneNodeParent parent);

    /// Copy Constructor.
    /// @param other    SceneNode to copy.
    explicit SceneNode(const SceneNode& other)
        : SceneNodeBase(other.m_manager), m_parent(other.m_parent), m_children(other.m_children), m_name(other.m_name)
    {}

public:
    /// No assignment.
    void operator=(const SceneNode&) = delete;

    /// Destructor
    ~SceneNode() override;

    /// The user-defined name of this SceneNode, is potentially empty and not guaranteed to be unique if set.
    const std::string& name() const { return m_name; }

    ///@{
    /// Iterateable container of all children of this node.
    Children children() { return Children(*this); }
    ConstChildren children() const { return ConstChildren(*this); }
    ///@}

    ///@{
    /// The parent of this SceneNode.
    valid_ptr<SceneNode*> parent() { return m_parent.get(); }
    valid_ptr<const SceneNode*> parent() const { return const_cast<SceneNode*>(this)->parent(); }
    ///@}

    /// Tests, if this SceneNode is a descendant of the given ancestor.
    bool has_ancestor(const SceneNode* ancestor) const;

    ///@{
    /// Finds and returns the first common ancestor of two SceneNodes, returns empty if none exists.
    risky_ptr<SceneNode*> common_ancestor(SceneNode* other);
    risky_ptr<const SceneNode*> common_ancestor(const SceneNode* other) const
    {
        return const_cast<SceneNode*>(this)->common_ancestor(const_cast<SceneNode*>(other));
    }
    ///@}

    ///@{
    /// Returns the first ancestor of this SceneNode that has a specific type (can be empty if none is found).
    template<typename Type>
    risky_ptr<Type*> first_ancestor() const
    {
        valid_ptr<const SceneNode*> next = parent();
        while (next != next->parent()) {
            if (const Type* result = dynamic_cast<Type*>(next)) {
                return result;
            }
            next = next->parent();
        }
        return {};
    }
    template<typename Type>
    risky_ptr<const Type*> first_ancestor() const
    {
        return const_cast<SceneNode*>(this)->first_ancestor<Type>();
    }
    ///@}

    /// Updates the name of this SceneNode.
    /// @param name New name of this SceneNode.
    const std::string& set_name(std::string name);

    /// Moves this SceneNode to the front of all of its siblings.
    void stack_front();

    /// Moves this SceneNode behind all of its siblings.
    void stack_back();

    /// Moves this SceneNode before a given sibling.
    /// @param sibling  Sibling to stack before.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_before(valid_ptr<SceneNode*> sibling);

    /// Moves this SceneNode behind a given sibling.
    /// @param sibling  Sibling to stack behind.
    /// @throws hierarchy_error If the sibling is not a sibling of this node.
    void stack_behind(valid_ptr<SceneNode*> sibling);

protected:
    /// Adds a new child SceneNode to the
    /// @param args     Arguments to create the new node with.
    template<typename T, typename... Args, typename = std::enable_if_t<SceneManager::is_scene_node_type<T>::value>>
    SceneNodeChild<T> _add_child(Args&&... args)
    {
        const Token token;
        return SceneManager::Access<SceneNode>(this).add_child<T>(token, std::forward<Args>(args)...);
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Non-owning pointer to the parent SceneNode.
    SceneNodeParent m_parent;

    /// All children of this SceneNode.
    std::vector<valid_ptr<SceneNode*>> m_children;

    /// The user-defined name of this SceneNode, is potentially empty and not guaranteed to be unique if set.
    std::string m_name;
};

NOTF_CLOSE_NAMESPACE
