#pragma once

#include <mutex>
#include <unordered_set>

#include "app/application.hpp"
#include "app/io/time.hpp"
#include "common/map.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thread-safe container of arbitrary values that can be connected using expressions.
/// Properties are owned by Items but stored in the PropertyGraph in a DAG (direct acyclic graph). Access to a value
/// in the graph is facilitated by a PropertyKey - consisting of an ItemId and PropertyId. It is the Item's
/// responsibility to make sure that all of its Properties are removed when it goes out of scope.
class PropertyGraph {

    template<typename T>
    friend class Property; // has access to the graph's types
    // TODO: remove friend - is only necessary for dependency vector and that shouldn't be exposed anyway

    // types ---------------------------------------------------------------------------------------------------------//
public:
    template<typename T> // TODO: restrict access to Property types
    class Access;

    /// Thrown when a Property with an unknown PropertyKey is requested.
    NOTF_EXCEPTION_TYPE(lookup_error)

    /// Thrown when an existing Property is asked for the wrong type of value.
    NOTF_EXCEPTION_TYPE(type_error)

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(cyclic_dependency_error)

    //=========================================================================
private:
    /// An untyped Property in the graph.
    class PropertyBase {

        // methods ------------------------------------------------------------
    public:
        /// Virtual destructor.
        virtual ~PropertyBase();

        /// Type of the Property.
        virtual const std::type_info& type() const = 0;

        /// All dependencies of this Property.
        const std::vector<PropertyBase*>& dependencies() const { return m_dependencies; }

        /// Prepares this Property for removal from the Graph.
        /// @param graph    Graph used to find affected Properties.
        void prepare_removal(PropertyGraph& graph)
        {
            _unregister_from_dependencies(graph);
            for (PropertyBase* affected : m_affected) {
                NOTF_ASSERT(graph.has_property(affected), "Accessed invalid Property \"" << affected << "\"");
                affected->_freeze(graph);
            }
        }

    protected:
        /// Removes the expression from this Property.
        /// @param graph        Graph used to find dependency Properties.
        virtual void _clear_expression(PropertyGraph& graph) = 0;

        /// Freezing a property means removing its expression without changing its value.
        /// @param graph        Graph used to find dependent Properties.
        void _freeze(PropertyGraph& graph)
        {
            _clear_expression(graph);
            _unregister_from_dependencies(graph);
            _set_clean();
        }

        /// Registers this property as affected with all of its dependencies.
        /// @param graph        Graph used to find dependency Properties.
        void _register_with_dependencies(PropertyGraph& graph)
        {
            for (PropertyBase* dependency : m_dependencies) {
                NOTF_ASSERT(graph.has_property(dependency), "Accessed invalid Property \"" << dependency << "\"");
                NOTF_ASSERT(std::find(dependency->m_affected.begin(), dependency->m_affected.end(), this)
                                == dependency->m_affected.end(),
                            "Duplicate insertion of Property \"" << this << "\" as affected of dependency: \""
                                                                 << dependency << "\"");
                dependency->m_affected.emplace_back(this);
            }
        }

        /// Removes all dependencies from this property.
        /// @param graph        Graph used to find dependency Properties.
        void _unregister_from_dependencies(PropertyGraph& graph)
        {
            for (PropertyBase* dependency : m_dependencies) {
                NOTF_ASSERT(graph.has_property(dependency), "Accessed invalid Property \"" << dependency << "\"");
                auto it = std::find(dependency->m_affected.begin(), dependency->m_affected.end(), this);
                NOTF_ASSERT(it != dependency->m_affected.end(),
                            "Failed to unregister previously registered affected Property \""
                                << this << "\" from dependency: \"" << dependency << "\"");
                *it = std::move(dependency->m_affected.back());
                dependency->m_affected.pop_back();
            }
            m_dependencies.clear();
        }

        /// Set all affected properties dirty.
        /// @param graph        Graph used to find affected Properties.
        void _set_affected_dirty(PropertyGraph& graph)
        {
            for (PropertyBase* affected : m_affected) {
                NOTF_ASSERT(graph.has_property(affected), "Accessed invalid Property \"" << affected << "\"");
                affected->_set_dirty();
            }
        }

        /// Checks if this Property is dirty.
        bool _is_dirty() const { return m_time.is_invalid(); }

        /// Sets the Property dirty.
        void _set_dirty() { m_time = Time::invalid(); }

        /// Sets the Property clean.
        /// @param time     Time when the decision to set the Property was made. Is `now` by default.
        void _set_clean(Time time = Time::invalid()) { m_time = (time.is_valid() ? std::move(time) : Time::now()); }

        // fields -------------------------------------------------------------
    protected:
        /// All Properties that this one depends on.
        std::vector<PropertyBase*> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<PropertyBase*> m_affected;

        /// Time when this Property was last defined.
        /// Is invalid if the Property is dirty (its expression needs to be evaluated).
        Time m_time = Time::now();
    };

    //=========================================================================

    /// A typed Property instance in the graph that can be connected to other Properties via expressions.
    template<typename T>
    class Property : public PropertyBase {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the PropertyType.
        Property(T&& value) : m_value(std::move(value)) {}

        /// Type of the Property.
        virtual const std::type_info& type() const override { return typeid(T); }

        /// The Property's value.
        /// If the Property is defined by an expression, this might evaluate the expression.
        /// @param graph        Graph used to (potentially) evaluate the expression of this Property in.
        /// @throws type_error  If the wrong value type is requested.
        /// @returns            Const reference to the value of the Property.
        const T& value(PropertyGraph& graph)
        {
            if (_is_dirty()) {
                _evaluate_expression(graph);
            }
            return m_value;
        }

        /// Set the Property's value.
        /// Removes an existing expression on the Property if it exists.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        /// @param value        New value.
        /// @throws type_error  If a value of the wrong type is passed.
        void set_value(PropertyGraph& graph, T&& value)
        {
            if (m_expression) {
                _freeze(graph);
            }
            if (value == m_value) {
                return;
            }
            m_value = std::forward<T>(value);
            _set_affected_dirty(graph);
        }

        /// Set property's expression.
        /// @param graph        Graph used to find affected Properties.
        /// @param expression   Expression to set.
        /// @param dependencies Properties that the expression depends on.
        /// @throws type_error  If an expression returning the wrong type is passed.
        void set_expression(PropertyGraph& graph, std::function<T(const PropertyGraph&)> expression,
                            std::vector<const PropertyBase*> dependencies)
        {
            if (expression == m_expression) {
                return;
            }
            if (!expression) {
                _freeze(graph);
                return;
            }

            _unregister_from_dependencies(graph);
            m_expression = std::move(expression);
            m_dependencies = std::move(dependencies);
            _register_with_dependencies(graph);

            _set_dirty();
            _set_affected_dirty(graph);
        }

    private:
        /// Removes the expression from this Property.
        /// @param graph        Graph used to find dependency Properties.
        void _clear_expression(PropertyGraph& graph) override
        {
            if (m_expression) {
                _evaluate_expression(graph);
                m_expression = {};
            }
        }

        /// Evaluates this Property's expression and updates its value.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find dependency Properties.
        void _evaluate_expression(PropertyGraph& graph)
        {
            NOTF_ASSERT(m_expression, "Cannot evaluate expression from frozen Property \"" << this << "\"");
            m_value = m_expression(graph);
            _set_clean();
        }

        // fields -------------------------------------------------------------
    private:
        /// Value held by the Property.
        T m_value;

        /// Expression evaluating to a new value for this Property.
        std::function<T(const PropertyGraph&)> m_expression;
    };

    //=========================================================================

    // TODO: implement PropertyDelta
    class PropertyDeltaBase {};

    //=========================================================================

    template<typename T>
    class PropertyDelta : public PropertyDeltaBase {};

    // methods -------------------------------------------------------------------------------------------------------//
private:
    /// Checks if the graph has a given Property.
    /// @param property     Property to check.
    /// @returns            True iff there is a Property with the requested key in the graph.
    bool has_property(const PropertyBase* property) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_properties.count(property) != 0;
    }

    /// Creates an new Property for a given Item and assigns its initial value (and type).
    /// @param value            Initial value of the Property, also used to determine its type.
    /// @returns                New Property.
    template<typename T>
    Property<T>* add_property(T&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto property = std::make_unique<Property<T>>(std::forward<T>(value));
        m_properties.emplace(std::make_pair(property.get(), std::move(property)));
    }

    /// Returns the value of the Property.
    /// @param property         Requested Property.
    /// @throws lookup_error    If a Property with the given id does not exist.
    /// @throws type_error      If the wrong value type is requested.
    /// @return                 Value of the property.
    template<typename T>
    const T& value(PropertyBase* property) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const Property<T>* typed_property = dynamic_cast<Property<T>>(_find_property(property));
        if (!typed_property) {
            _throw_wrong_type(property, typeid(T), typeid(typed_property));
        }
        return typed_property->value(*this);
    }

    /// Sets the value of a Property.
    /// @param property         Property to modify.
    /// @param value            New value.
    /// @throws lookup_error    If a Property with the given id does not exist.
    /// @throws type_error      If the wrong value type is requested.
    template<typename T>
    void set_value(PropertyBase* property, T&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const Property<T>* typed_property = dynamic_cast<Property<T>>(_find_property(property));
        if (!property) {
            _throw_wrong_type(property, typeid(T), typeid(typed_property));
        }
        typed_property->set_value(*this, std::forward<T>(value));
    }

    /// Sets the expression of a Property.
    /// @param property         Property to modify.
    /// @param expression       New expression defining the value of the property.
    /// @param dependencies     Keys of all dependencies. It is of critical importance, that all properties in the
    ///                         expression are listed.
    /// @throws lookup_error    If a Property with the given id or one of the dependencies does not exist.
    /// @throws type_error      If the expression returns the wrong type.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    template<typename T>
    void set_expression(PropertyBase* property, std::function<T(const PropertyGraph&)>&& expression,
                        std::vector<PropertyBase*> dependencies)
    {
        static_assert(!std::is_same<T, void>::value, "Cannot define a Property with an expression returning `void`");
        std::lock_guard<std::mutex> lock(m_mutex);
        _detect_cycles(property, dependencies);
        const Property<T>* typed_property = dynamic_cast<Property<T>>(_find_property(property));
        if (!typed_property) {
            _throw_wrong_type(property, typeid(T), typeid(typed_property));
        }
        typed_property->set_expression(*this, std::move(expression), std::move(dependencies));
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the PropertyKey does not identify a Property in the graph, the call is silently ignored.
    /// @param property     Property to delete.
    void delete_property(const PropertyBase* property);

private:
    /// Property access with proper error reporting.
    /// @param property         Property to find.
    /// @throws lookup_error    If the requested Property does not exist in the graph.
    /// @returns                The requested Property.
    PropertyBase* _find_property(PropertyBase* property)
    {
        if (m_properties.find(property) == m_properties.end()) {
            _throw_not_found(property);
        }
        return property;
    }
    const PropertyBase* _find_property(PropertyBase* property) const
    {
        return const_cast<PropertyGraph*>(this)->_find_property(property);
    }

    /// Detects potential cyclic dependencies in the graph before they are introduced.
    /// @param property     Property that receives a new expression.
    /// @param dependencies All Properties that `property` will potentially be dependent on.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    void _detect_cycles(const PropertyBase* property, const std::vector<PropertyBase*>& dependencies);

    /// Call when a PropertyKey is not found.
    NOTF_NORETURN void _throw_not_found(const PropertyBase* property) const;

    /// Call when a Property is requested with the wrong type.
    NOTF_NORETURN void
    _throw_wrong_type(const PropertyBase* property, const std::type_info& expected, const std::type_info& actual) const;

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All properties in the graph.
    robin_map<const PropertyBase*, std::unique_ptr<PropertyBase>> m_properties;

    /// The current delta.
    robin_map<const PropertyBase*, std::unique_ptr<PropertyDeltaBase>> m_delta;

    /// Mutex guarding the graph.
    mutable std::mutex m_mutex;
};

// ===================================================================================================================//

/// An object managing a single Property in the PropertyGraph.
template<typename T>
class Property {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    NOTF_NO_COPY_OR_ASSIGN(Property)

    /// Constructor.
    /// @param value    Initial value of the Property.
    Property(T&& value) { m_pointer = PropertyGraph::Access<Property<T>>::create_property(std::forward<T>(value)); }

    /// Move Constructor.
    /// @param other    Property to move from.
    Property(Property&& other) : m_pointer(other.m_pointer) { other.m_pointer = nullptr; }

    /// Destructor.
    ~Property()
    {
        if (m_pointer) {
            PropertyGraph::Access<Property<T>>().delete_property(m_pointer);
            m_pointer = nullptr;
        }
    }

    /// The value of this Property.
    /// @throws lookup_error    If you try to access a Property that was moved from.
    const T& value() { return PropertyGraph::Access<Property<T>>(m_pointer).value(); }
    // TODO: we know that pointer is valid ... that's why we are doing this in the first place!
    // TODO: implement this in Item

    /// Sets the value of this Property.
    /// @param value            New value.
    /// @throws lookup_error    If you try to modify a Property that was moved from.
    void set_value(T&& value) { PropertyGraph::Access<Property<T>>(m_pointer).set_value(std::forward<T>(value)); }

    /// Sets an expression of this Property.
    /// @param expression       New expression defining the value of the property.
    /// @param dependencies     Keys of all dependencies. It is of critical importance, that all properties in the
    ///                         expression are listed.
    /// @throws lookup_error    If you try to modify a Property that was moved from.
    void set_expression(std::function<T(const PropertyGraph&)>&& expression,
                        std::vector<PropertyGraph::PropertyBase*> dependencies)
    {
        PropertyGraph::Access<Property<T>>(m_pointer).set_expression(std::move(expression), std::move(dependencies));
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Raw pointer to the Property in the graph.
    T* m_pointer = nullptr;
};

//====================================================================================================================//

template<typename T>
class PropertyGraph::Access<Property<T>> {
    friend class Property<T>;

    /// Constructor.
    /// @param property     Property to allow access.
    Access(PropertyBase* property) : m_property(property)
    {
        if (!m_property) {
            notf_throw(PropertyGraph::lookup_error, "Cannot access a Property that was moved from");
        }
    }

    /// Creates an new Property and assigns its initial value (and type).
    /// @param value    Initial value of the Property, also used to determine its type.
    static Property<T>* create_property(T&& value)
    {
        return NOTF_APP().property_graph().add_property(std::forward<T>(value));
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the given pointer does not identify a Property in the graph, the call is silently ignored.
    /// @param property     Property to delete.
    void delete_property() { NOTF_APP().property_graph().delete_property(m_property); }

    /// The value of this Property.
    const T& value() { return NOTF_APP().property_graph().value<T>(m_property); }

    /// Sets the value of this Property.
    /// @param value    New value.
    void set_value(T&& value) { NOTF_APP().property_graph().set_value(m_property, std::forward<T>(value)); }

    /// Sets an expression of this Property.
    /// @param expression       New expression defining the value of the property.
    /// @param dependencies     Keys of all dependencies. It is of critical importance, that all properties in the
    ///                         expression are listed.
    void set_expression(std::function<T(const PropertyGraph&)>&& expression, std::vector<PropertyBase*> dependencies)
    {
        NOTF_APP().property_graph().set_expression(m_property, std::move(expression), std::move(dependencies));
    }

    /// Property to allow access.
    PropertyBase* m_property;
};

NOTF_CLOSE_NAMESPACE
