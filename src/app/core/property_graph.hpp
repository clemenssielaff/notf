#pragma once

#include <cassert>
#include <mutex>
#include <unordered_map>

#include "app/ids.hpp"
#include "common/any.hpp"
#include "common/exception.hpp"
#include "common/variant.hpp"
#include "utils/bitsizeof.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// A PropertyKey is used to identify a single Property in the PropertyGraph.
/// It consists of both the ID of the Property itself, as well as that of its Item.
struct PropertyKey {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// No default constructor.
    PropertyKey() = delete;

    /// Value constructor.
    /// @param item_id      ID of the Item owning the Property.
    /// @param property_id  ID of the Property in the Item's PropertyGroup.
    PropertyKey(ItemId item_id, PropertyId property_id) : m_item_id(item_id), m_property_id(property_id) {}

    /// ID of the Item owning the Property.
    ItemId item_id() const { return m_item_id; }

    /// ID of the Property in the Item's PropertyGroup.
    PropertyId property_id() const { return m_property_id; }

    /// Explicit invalid PropertyKey generator.
    static PropertyKey invalid() { return PropertyKey(ItemId::INVALID, PropertyId::INVALID); }

    /// Equality operator.
    /// @param rhs  PropertyKey to test against.
    bool operator==(const PropertyKey& rhs) const
    {
        return m_item_id == rhs.m_item_id && m_property_id == rhs.m_property_id;
    }

    /// Inequality operator.
    /// @param rhs  PropertyKey to test against.
    bool operator!=(const PropertyKey& rhs) const
    {
        return m_item_id != rhs.m_item_id || m_property_id != rhs.m_property_id;
    }

    /// Lesser-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator<(const PropertyKey& rhs) const
    {
        if (m_item_id < rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id < rhs.m_property_id;
        }
    }

    /// Lesser-or-equal-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator<=(const PropertyKey& rhs) const
    {
        if (m_item_id < rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id <= rhs.m_property_id;
        }
    }

    /// Greater-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator>(const PropertyKey& rhs) const
    {
        if (m_item_id > rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id > rhs.m_property_id;
        }
    }

    /// Greater-or-equal-than operator.
    /// @param rhs  PropertyKey to test against.
    bool operator>=(const PropertyKey& rhs) const
    {
        if (m_item_id > rhs.m_item_id) {
            return true;
        }
        else {
            return m_item_id == rhs.m_item_id && m_property_id >= rhs.m_property_id;
        }
    }

    /// Checks if this PropertyKey is valid or not.
    bool is_valid() const { return m_item_id != ItemId::INVALID && m_property_id != PropertyId::INVALID; }

    /// Checks if this PropertyKey is valid or not.
    explicit operator bool() const { return is_valid(); }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// ID of the Item owning the Property.
    ItemId m_item_id;

    /// ID of the Property in the Item's PropertyGroup.
    PropertyId m_property_id;
};

//====================================================================================================================//

/// Prints a human-readable representation of a PropertyKey into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param key  PropertyKey to print.
/// @return Output stream for further output.
inline std::ostream& operator<<(std::ostream& out, const PropertyKey& key)
{
    return out << "(" << key.item_id() << ", " << key.property_id() << ")";
}

NOTF_CLOSE_NAMESPACE

//== std::hash =======================================================================================================//

namespace std {

template<>
struct hash<notf::PropertyKey> {
    size_t operator()(const notf::PropertyKey& key) const
    {
        return (static_cast<size_t>(key.item_id().value()) << (notf::bitsizeof<size_t>() / 2))
               ^ key.property_id().value();
    }
};

} // namespace std

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// Thread-safe container of arbitrary values that can be connected using expressions.
/// Properties are owned by Items but stored in the PropertyGraph. Access to a value in the graph is facilitated by a
/// PropertyKey - consisting of an ItemId and PropertyId. It is the Item's responsibility to make sure that all of its
/// Properties are removed when it goes out of scope.
class PropertyGraph {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Item)

    /// Thrown when a Property with an unknown PropertyKey is requested.
    NOTF_EXCEPTION_TYPE(lookup_error)

    /// Thrown when an existing Property is asked for the wrong type of value.
    NOTF_EXCEPTION_TYPE(type_error)

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(cyclic_dependency_error)

    /// Thrown when a PropertyGroup would be created twice or could not found (internal error).
    NOTF_EXCEPTION_TYPE(group_error)

private:
    /// The PropertyGroup is created by an Item with the key: (ItemId, 0) and holds information about Properties in the
    /// group as well as the GraphicsProducer associated with the Item.
    class PropertyGroup {
        // methods ------------------------------------------------------------
    public:
        /// Default constructor.
        PropertyGroup(GraphicsProducerId graphics_producer) : m_item_producer(std::move(graphics_producer)) {}

        /// All members of this group.
        const std::vector<PropertyKey>& members() const { return m_members; }

        /// Returns the next PropertyId in this group.
        PropertyId next_property_id() { return m_group_counter++; }

        /// Adds a new Property to this group.
        /// @param key  Key of the Property to add.
        void add_member(PropertyKey key);

        /// Removes a Property as member of this Group.
        /// @param key  Key of the Property to remove.
        void remove_member(const PropertyKey& key);

        // fields -------------------------------------------------------------
    private:
        /// All Properties in this group.
        std::vector<PropertyKey> m_members;

        /// The GraphicsProducer associated with the Item of this PropertyGroup.
        const GraphicsProducerId m_item_producer;

        /// Counter used by PropertyGroups to generate new PropertyIds for members of the group.
        PropertyId::underlying_t m_group_counter{1};
    };

    //=========================================================================

    /// An untyped Property in the graph that can be connected to other Properties via expressions.
    /// Uses std::any for internal type erasure of the Property value.
    class Property {

        // methods ------------------------------------------------------------
    public:
        /// Value constructor.
        /// @param value    Value held by the Property, is used to determine the PropertyType.
        template<typename T>
        Property(T&& value) : m_value(std::move(value))
        {}

        /// The Property's value.
        /// If the Property is defined by an expression, this might evaluate the expression.
        /// @param my_key       PropertyKey of this Property.
        /// @throws type_error  If the wrong value type is requested.
        /// @returns            Const reference to the value of the Property.
        template<typename T>
        const T& value(const PropertyKey& my_key, PropertyGraph& graph)
        {
            _assert_correct_type(my_key, typeid(T));
            if (m_is_dirty) {
                _evaluate_expression(my_key, graph);
            }
            const T* result = std::any_cast<T>(&m_value);
            assert(result);
            return *result;
        }

        /// Set the Property's value.
        /// Removes an existing expression on the Property if it exists.
        /// @param my_key       PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        /// @param value        New value.
        /// @throws type_error  If a value of the wrong type is passed.
        template<typename T>
        void set_value(const PropertyKey& my_key, PropertyGraph& graph, T&& value)
        {
            _assert_correct_type(my_key, typeid(T));
            _freeze(my_key, graph);
            if (value != *std::any_cast<T>(&m_value)) {
                m_value = std::forward<T>(value);
                _set_affected_dirty(graph);
            }
        }

        /// Set property's expression.
        /// @param my_key       PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        /// @param expression   Expression to set.
        /// @param dependencies Properties that the expression depends on.
        /// @throws type_error  If an expression returning the wrong type is passed.
        template<typename T>
        void set_expression(const PropertyKey& my_key, PropertyGraph& graph,
                            std::function<T(const PropertyGraph&)> expression, std::vector<PropertyKey> dependencies)
        {
            _assert_correct_type(my_key, typeid(T));
            _unregister_from_dependencies(my_key, graph);

            // wrap the typed expression into one that returns a std::any
            m_expression
                = [expr = std::move(expression)](const PropertyGraph& graph) -> std::any { return expr(graph); };
            m_dependencies = std::move(dependencies);
            m_is_dirty = true;

            _register_with_dependencies(my_key, graph);
            _set_affected_dirty(graph);
        }

        /// All dependencies of this Property.
        const std::vector<PropertyKey>& dependencies() const { return m_dependencies; }

        /// Prepares this Property for removal from the Graph.
        /// @param my_key       PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        void prepare_removal(const PropertyKey& my_key, PropertyGraph& graph);

    private:
        /// Freezing a property means removing its expression without changing its value.
        /// @param my_key       PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        void _freeze(const PropertyKey& my_key, PropertyGraph& graph) noexcept
        {
            _unregister_from_dependencies(my_key, graph);
            m_expression = {};
            m_is_dirty = false;
        }

        /// Evaluates this Property's expression and updates its value.
        /// @param my_key       PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        void _evaluate_expression(const PropertyKey& my_key, PropertyGraph& graph);

        /// Registers this property as affected with all of its dependencies.
        /// @param my_key   PropertyKey of this Property.
        /// @param graph    Graph used to find dependency Properties.
        void _register_with_dependencies(const PropertyKey& my_key, PropertyGraph& graph);

        /// Removes all dependencies from this property.
        /// @param my_key   PropertyKey of this Property.
        /// @param graph    Graph used to find dependent Properties.
        void _unregister_from_dependencies(const PropertyKey& my_key, PropertyGraph& graph);

        /// Set all affected properties dirty.
        /// @param graph    Graph used to find affected Properties.
        void _set_affected_dirty(PropertyGraph& graph);

        /// Makes sure that the given type_info is the same as the one of the Property's value.
        /// @param my_key       PropertyKey of this Property.
        /// @param info         Type to check.
        /// @throws type_error  If the wrong value type is requested.
        void _assert_correct_type(const PropertyKey& my_key, const std::type_info& info);

        // fields -------------------------------------------------------------
    private:
        /// Value held by the Property.
        std::any m_value;

        /// Expression evaluating to a new value for this Property.
        std::function<std::any(const PropertyGraph&)> m_expression;

        /// All Properties that this one depends on.
        std::vector<PropertyKey> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<PropertyKey> m_affected;

        /// Whether the Property is dirty (its expression needs to be evaluated).
        bool m_is_dirty = false;

#ifdef NOTF_DEBUG
        char _padding[7];
#endif
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// Checks if the graph has a Property with the given key.
    /// @param key  Key of the Property to search for.
    /// @returns    True iff there is a Property with the requested key in the graph.
    bool has_property(const PropertyKey key) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_properties.count(key) != 0;
    }

    /// Returns the value of the Property requested by type and key.
    /// @param key              Key of the requested Property.
    /// @throws lookup_error    If a Property with the given id does not exist.
    /// @throws type_error      If the wrong value type is requested.
    /// @return                 Value of the property.
    template<typename T>
    const T& value(const PropertyKey key) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return _find_property(key).value<T>(key, *this);
    }

    /// Sets the value of a Property.
    /// @param key              Key of the Property to modify.
    /// @param value            New value.
    /// @throws lookup_error    If a Property with the given id does not exist.
    /// @throws type_error      If the wrong value type is requested.
    template<typename T>
    void set_property(const PropertyKey key, T&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        _find_property(key).set_value(key, *this, std::forward<T>(value));
    }

    /// Sets the expression of a Property.
    /// @param key              Key of the Property to modify.
    /// @param expression       New expression defining the value of the property.
    /// @param dependencies     Keys of all dependencies. It is of critical importance, that all properties in the
    ///                         expression are listed.
    /// @throws lookup_error    If a Property with the given id or one of the dependencies does not exist.
    /// @throws type_error      If the expression returns the wrong type.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    template<typename T>
    void set_property(const PropertyKey key, std::function<T(const PropertyGraph&)>&& expression,
                      std::vector<PropertyKey> dependencies)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        _detect_cycles(key, dependencies);
        _find_property(key).set_expression(key, *this, std::move(expression), std::move(dependencies));
    }

private: // for Access<Item>
    /// Creates a new PropertyGroup.
    /// @param item_id      Item of this PropertyGroup.
    /// @param produder     The GraphicsProducer associated with the Item.
    /// @throws group_error If a PropertyGroup for the Item already exists.
    void _add_group(const ItemId item_id, GraphicsProducerId producer);

    /// Creates an new Property for a given Item and assigns its initial value (and type).
    /// @param item_id  Item for which to construct the new Property.
    /// @param value    Initial value of the Property, also used to determine its type.
    /// @returns        Key of the new Property. Is invalid, if the given Item id is invalid.
    template<typename T>
    PropertyKey _add_property(const ItemId item_id, T&& value)
    {
        if (!item_id.is_valid()) {
            return PropertyKey::invalid();
        }
        PropertyKey new_key = PropertyKey::invalid();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            PropertyGroup& group = _group_for(item_id);
            new_key = PropertyKey(item_id, group.next_property_id());
            auto result = m_properties.emplace(std::make_pair(new_key, Property(std::forward<T>(value))));
            assert(result.second);
            group.add_member(new_key);
        }
        return new_key;
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the PropertyKey does not identify a Property in the graph, the call is silently ignored.
    void _delete_property(const PropertyKey key);

    /// Deletes a PropertyGroup from the graph.
    /// @param id   ItemId used to identify the group.
    /// If the id does not identify a PropertyGroup in the graph, the call is silently ignored.
    void _delete_group(const ItemId id);

private:
    /// Property access with proper error reporting.
    /// @param key              Key of the Property to find.
    /// @throws lookup_error    If a property with the requested key does not exist in the graph.
    /// @returns                The requested Property.
    Property& _find_property(const PropertyKey key)
    {
        if (key.property_id().is_valid()) { // you cannot request the implicit PropertyGroup properties
            const auto it = m_properties.find(key);
            if (it != m_properties.end()) {
                assert(std::holds_alternative<Property>(it->second));
                return std::get<Property>(it->second);
            }
        }
        _throw_notfound(key);
    }
    const Property& _find_property(const PropertyKey key) const
    {
        return const_cast<PropertyGraph*>(this)->_find_property(key);
    }

    /// Call when a PropertyKey is not found.
    NOTF_NORETURN void _throw_notfound(const PropertyKey key);

    /// Creates a new or returns an existing group Property for the given Item.
    /// @param item_id      PropertyGroups are identified by the ItemId alone.
    /// @throws group_error If a PropertyGroup for the Item already exists.
    PropertyGroup& _group_for(const ItemId item_id);

    /// Detects potential cyclic dependencies in the graph before they are introduced.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    void _detect_cycles(const PropertyKey key, const std::vector<PropertyKey>& dependencies);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All properties in the graph.
    std::unordered_map<PropertyKey, std::variant<PropertyGroup, Property>> m_properties;

    /// Mutex guarding the graph.
    mutable std::mutex m_mutex;
};

// ===================================================================================================================//

template<>
class PropertyGraph::Access<Item> {
    friend class Item;

    /// Constructor.
    /// @param context  PropertyGraph to access.
    /// @param item     Id of the Item that created this instance.
    Access(PropertyGraph& graph, const ItemId item) : m_graph(graph), m_item(item) {}

    /// Creates a new PropertyGroup.
    /// @param produder     The GraphicsProducer associated with the Item.
    /// @throws group_error If a PropertyGroup for the Item already exists.
    void add_group(GraphicsProducerId producer) { m_graph._add_group(m_item, std::move(producer)); }

    /// Creates an new Property for a given Item and assigns its initial value (and type).
    /// @param item_id  Item for which to construct the new Property.
    /// @param value    Initial value of the Property, also used to determine its type.
    /// @returns        Key of the new Property. Is invalid, if the given Item id is invalid.
    template<typename T>
    TypedPropertyId<T> add_property(T&& value)
    {
        return m_graph._add_property(m_item, std::forward<T>(value)).property_id();
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the PropertyKey does not identify a Property in the graph, the call is silently ignored.
    /// @param property_id  Id of the Property to delete.
    void delete_property(const PropertyId property_id) { m_graph._delete_property(PropertyKey(m_item, property_id)); }

    /// Deletes a PropertyGroup from the graph.
    /// If the Item id does not identify a PropertyGroup in the graph, the call is silently ignored.
    void delete_group() { m_graph._delete_group(m_item); }

    /// The PropertyGraph to access.
    PropertyGraph& m_graph;

    /// Id of the Item that created this instance.
    const ItemId m_item;
};

NOTF_CLOSE_NAMESPACE
