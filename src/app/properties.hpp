#pragma once

#include <set>

#include "app/forwards.hpp"
#include "common/exception.hpp"
#include "common/hash.hpp"
#include "common/mutex.hpp"
#include "common/pointer.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

class PropertyGraph : public std::enable_shared_from_this<PropertyGraph> {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(SceneNode)

    /// Checks if T is a valid type to store in a Property.
    template<typename T>
    using is_property_type = typename std::conjunction<std::is_copy_constructible<T>, std::negation<std::is_const<T>>>;

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(no_dag_error)

    template<typename, typename>
    class TypedPropertyBody; // forward

    template<typename>
    class PropertyHead; // forward

    //=========================================================================
private:
    class PropertyBody {

        template<typename, typename>
        friend class TypedPropertyBody;

        // methods ------------------------------------------------------------
    public:
        /// Constructor.
        /// @param graph    PropertyGraph containing this Property.
        /// @param node     SceneNode associated with this Property.
        PropertyBody(PropertyGraph& graph, SceneNode* node) : m_graph(graph), m_node(node) {}

        /// Destructor.
        virtual ~PropertyBody();

        /// PropertyGraph containing this Property.
        PropertyGraph& graph() { return m_graph; }

    protected:
        /// Updates the Property by evaluating its expression.
        /// Then continues to update all downstream nodes as well.
        virtual void _update() = 0;

        /// Removes this Property as affected (downstream) of all of its dependencies (upstream).
        void _ground();

        /// Checks that none of the proposed upstream Properties:
        ///     * is still alive (return false)
        ///     * is already downstream of this one (no_dag exception).
        ///     * has this one as a child already (assert).
        /// @returns False if one of the dependencies has since been removed.
        bool _validate_upstream(const std::vector<valid_ptr<PropertyBody*>>& dependencies) const;

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        std::vector<valid_ptr<PropertyBody*>> m_upstream;

        /// Properties affected by this one through expressions.
        std::vector<valid_ptr<PropertyBody*>> m_downstream;

        /// Graph containing this Property.
        PropertyGraph& m_graph;

        /// SceneNode associated with this Property (can be nullptr if this is an unassociated Property).
        risky_ptr<SceneNode*> m_node;
    };

    //=========================================================================
public:
    template<typename T, typename = std::enable_if_t<is_property_type<T>::value>>
    class TypedPropertyBody final : public PropertyBody {

        template<typename>
        friend class PropertyHead;

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param graph    PropertyGraph containing this Property.
        /// @param value    Value held by the Property, is used to determine the property type.
        /// @param node     SceneNode associated with this Property.
        TypedPropertyBody(PropertyGraph& graph, T value, SceneNode* node)
            : PropertyBody(graph, node), m_value(std::move(value))
        {}

        /// The Property's value.
        const T& value()
        {
            std::unique_lock<Mutex> lock(m_graph.m_mutex);
            return m_value;
        }

        /// Set the Property's value and updates downstream Properties.
        /// Removes an existing expression on this Property if one exists.
        /// @param value    New value.
        void set_value(T&& value)
        {
            std::unique_lock<Mutex> lock(m_graph.m_mutex);
            if (m_expression) {
                _ground();
            }
            _set_value(std::forward<T>(value));
        }

        /// Set the Property's expression.
        /// @param expression       Expression to set.
        /// @param dependencies     Properties that the expression depends on.
        /// @throws no_dag_error    If the expression would introduce a cyclic dependency into the graph.
        void set_expression(std::function<T()> expression, std::vector<valid_ptr<PropertyBody*>> dependencies)
        {
            if (expression) {
                std::unique_lock<Mutex> lock(m_graph.m_mutex);
                _ground();
                if (!_validate_upstream(dependencies)) {
                    return;
                }
                m_expression = std::move(expression);
                m_upstream = std::move(dependencies);
                for (valid_ptr<PropertyBody*> dependency : m_upstream) {
                    dependency->m_downstream.emplace_back(this);
                }
                _update();
            }
        }

    private:
        /// Updates the Property by evaluating its expression.
        void _update() override
        {
            NOTF_ASSERT(m_graph.m_mutex.is_locked_by_this_thread());
            if (m_expression) {
                _set_value(m_expression());
            }
        }

        /// Changes the value of this Property if the new one is different and then updates all affected Properties.
        void _set_value(T&& value)
        {
            NOTF_ASSERT(m_graph.m_mutex.is_locked_by_this_thread());
            if (value != m_value) {
                m_value = std::forward<T>(value);

                if (m_node) {
                    // TODO: Generate PropertyEvent here
                }

                for (valid_ptr<PropertyBody*> affected : m_downstream) {
                    affected->_update();
                }
            }
        }

        // fields -------------------------------------------------------------
    private:
        /// Expression evaluating to a new value for this Property.
        std::function<T()> m_expression;

        /// Value held by the Property.
        T m_value;
    };

    //=========================================================================

    template<typename T>
    class PropertyHead {
        friend class PropertyGraph;

        template<typename, typename>
        friend class TypedPropertyBody;

        // methods ------------------------------------------------------------
    private:
        NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

        /// Constructor.
        /// @param body     Body of the Property.
        PropertyHead(valid_ptr<TypedPropertyBody<T>*> body) : m_body(body), m_graph(body->graph().shared_from_this()) {}

        /// Factory method.
        /// @param body     Body of the Property.
        static std::shared_ptr<PropertyHead<T>> create(valid_ptr<TypedPropertyBody<T>*> body)
        {
            return NOTF_MAKE_SHARED_FROM_PRIVATE(PropertyHead, body);
        }

    public:
        /// Destructor.
        ~PropertyHead()
        {
            if (auto graph = m_graph.lock()) {
                graph->_delete_property(m_body);
            }
        }

        // TODO: get
        // TODO: set

        // fields -------------------------------------------------------------
    private:
        /// Body of the Property.
        valid_ptr<TypedPropertyBody<T>*> m_body;

        /// PropertyGraph containing the Property's body.
        std::weak_ptr<PropertyGraph> m_graph;
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Constructor.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    PropertyGraph(SceneGraph& scene_graph) : m_scene_graph(scene_graph) {}

public:
    /// Factory method.
    /// @param scene_graph  SceneGraph to deliver the PropertyEvents to.
    static PropertyGraphPtr create(SceneGraph& scene_graph)
    {
        return NOTF_MAKE_SHARED_FROM_PRIVATE(PropertyGraph, scene_graph);
    }

    /// Creates a new free Property in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    template<typename T>
    std::shared_ptr<PropertyHead<T>> create_property(T&& value)
    {
        return _create_property(std::forward<T>(value));
    }

private:
    /// Creates a new Property in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    /// @param node     SceneNode associated with this Property.
    template<typename T>
    std::shared_ptr<PropertyHead<T>> _create_property(T&& value, SceneNode* node = nullptr)
    {
        static_assert(is_property_type<T>::value, "T is not a valid Property type");
        auto body = std::make_unique<TypedPropertyBody<T>>(*this, std::forward<T>(value), node);
        auto head = PropertyHead<T>::create(body.get());
        {
            std::lock_guard<Mutex> lock(m_mutex);
            m_properties.emplace(std::move(body));
        }
        return head;
    }

    /// Deletes a Property from the graph.
    /// @param property     Property to remove.
    void _delete_property(const valid_ptr<PropertyBody*> property)
    {
        NOTF_ASSERT(&property->graph() == this);
        std::lock_guard<Mutex> lock(m_mutex);
        auto it = m_properties.find(property);
        if (it != m_properties.end()) {
            m_properties.erase(it);
        }
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Mutex guarding the graph and all Properties in it.
    Mutex m_mutex;

    /// All Property bodies in this graph.
    std::set<std::unique_ptr<PropertyBody>, pointer_equal<PropertyBody>> m_properties;

    /// SceneGraph to deliver the PropertyEvents to.
    SceneGraph& m_scene_graph;
};

/// Convenience typdef for PropertyHead shared pointers.
template<typename T>
using PropertyPtr = std::shared_ptr<PropertyGraph::PropertyHead<T>>;

/// Access for SceneNodes.
template<>
class PropertyGraph::Access<SceneNode> {
    friend class SceneNode;

    /// Constructor.
    /// @param graph    PropertyGraph to access.
    Access(PropertyGraph& graph) : m_graph(graph) {}

    /// Creates a new Property node in the graph.
    /// @param value    Value held by the Property, is used to determine the property type.
    /// @param node     SceneNode associated with this Property.
    template<typename T>
    PropertyPtr<T> create_property(T&& value, SceneNode* node)
    {
        return m_graph._create_property(std::forward<T>(value), node);
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// PropertyGraph to access.
    PropertyGraph& m_graph;
};

//====================================================================================================================//

NOTF_CLOSE_NAMESPACE
