#pragma once

#include <cassert>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "app/ids.hpp"
#include "app/io/time.hpp"
#include "common/any.hpp"
#include "common/exception.hpp"
#include "utils/bitsizeof.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

/// A PropertyKey is used to identify a single Property in the PropertyManager.
/// It consists of both the ID of the Property itself, as well as that of its Item.
struct PropertyKey {

    // methods -------------------------------------------------------------------------------------------------------//
public:
    /// No default constructor.
    PropertyKey() = delete;

    /// Value constructor.
    /// @param item_id      ID of the Item owning the Property.
    /// @param property_id  ID of the Property within its Item.
    PropertyKey(ItemId item_id, PropertyId property_id) : m_item_id(item_id), m_property_id(property_id) {}

    /// ID of the Item owning the Property.
    ItemId item_id() const { return m_item_id; }

    /// ID of the Property within its Item.
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

    /// ID of the Property within its Item.
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
/// Properties are owned by Items but stored in the PropertyManager in a DAG (direct acyclic graph). Access to a value
/// in the graph is facilitated by a PropertyKey - consisting of an ItemId and PropertyId. It is the Item's
/// responsibility to make sure that all of its Properties are removed when it goes out of scope.
class PropertyManager {

    // types ---------------------------------------------------------------------------------------------------------//
public:
    NOTF_ACCESS_TYPES(Item)

    /// Thrown when a Property with an unknown PropertyKey is requested.
    NOTF_EXCEPTION_TYPE(lookup_error)

    /// Thrown when an existing Property is asked for the wrong type of value.
    NOTF_EXCEPTION_TYPE(type_error)

    /// Thrown when a new expression would introduce a cyclic dependency into the graph.
    NOTF_EXCEPTION_TYPE(cyclic_dependency_error)

private:
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
        /// @param key          PropertyKey of this Property.
        /// @throws type_error  If the wrong value type is requested.
        /// @returns            Const reference to the value of the Property.
        template<typename T>
        const T& value(const PropertyKey& key, PropertyManager& graph)
        {
            _assert_correct_type(key, typeid(T));
            if (_is_dirty()) {
                _evaluate_expression(key, graph);
            }
            const T* result = std::any_cast<T>(&m_value);
            assert(result);
            return *result;
        }

        /// Set the Property's value.
        /// Removes an existing expression on the Property if it exists.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        /// @param value        New value.
        /// @throws type_error  If a value of the wrong type is passed.
        template<typename T>
        void set_value(const PropertyKey& key, PropertyManager& graph, T&& value)
        {
            _assert_correct_type(key, typeid(T));
            if (m_expression) {
                _freeze(key, graph);
            }
            if (value == *std::any_cast<T>(&m_value)) {
                return;
            }
            m_value = std::forward<T>(value);
            graph.m_dirty_items.insert(key.item_id());
            _set_affected_dirty(key, graph);
        }

        /// Set property's expression.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        /// @param expression   Expression to set.
        /// @param dependencies Properties that the expression depends on.
        /// @throws type_error  If an expression returning the wrong type is passed.
        template<typename T>
        void set_expression(const PropertyKey& key, PropertyManager& graph,
                            std::function<T(const PropertyManager&)> expression, std::vector<PropertyKey> dependencies)
        {
            if (expression == m_expression) {
                return;
            }
            if (!expression) {
                _freeze(key, graph);
                return;
            }

            _assert_correct_type(key, typeid(T));
            _unregister_from_dependencies(key, graph);

            m_expression = std::move(expression);
            m_dependencies = std::move(dependencies);
            _set_dirty(key, graph);

            _register_with_dependencies(key, graph);
            _set_affected_dirty(key, graph);
        }

        /// All dependencies of this Property.
        const std::vector<PropertyKey>& dependencies() const { return m_dependencies; }

        /// Prepares this Property for removal from the Graph.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        void prepare_removal(const PropertyKey& key, PropertyManager& graph);

    private:
        /// Freezing a property means removing its expression without changing its value.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find dependent Properties.
        void _freeze(const PropertyKey& key, PropertyManager& graph) noexcept
        {
            _unregister_from_dependencies(key, graph);
            m_expression = {};
            _set_clean();
        }

        /// Evaluates this Property's expression and updates its value.
        /// @param key          PropertyKey of this Property.
        /// @param graph        Graph used to find affected Properties.
        void _evaluate_expression(const PropertyKey& key, PropertyManager& graph);

        /// Registers this property as affected with all of its dependencies.
        /// @param key      PropertyKey of this Property.
        /// @param graph    Graph used to find dependency Properties.
        void _register_with_dependencies(const PropertyKey& key, PropertyManager& graph);

        /// Removes all dependencies from this property.
        /// @param key      PropertyKey of this Property.
        /// @param graph    Graph used to find dependent Properties.
        void _unregister_from_dependencies(const PropertyKey& key, PropertyManager& graph);

        /// Set all affected properties dirty.
        /// @param key      PropertyKey of this Property.
        /// @param graph    Graph used to find affected Properties.
        void _set_affected_dirty(const PropertyKey& key, PropertyManager& graph);

        /// Makes sure that the given type_info is the same as the one of the Property's value.
        /// @param key          PropertyKey of this Property.
        /// @param info         Type to check.
        /// @throws type_error  If the wrong value type is requested.
        void _assert_correct_type(const PropertyKey& key, const std::type_info& info);

        /// Checks if this Property is dirty.
        bool _is_dirty() const { return m_time.is_invalid(); }

        /// Sets the Property dirty.
        /// @param key      PropertyKey of this Property.
        /// @param graph    Graph storing dirty Items.
        void _set_dirty(const PropertyKey& key, PropertyManager& graph)
        {
            graph.m_dirty_items.insert(key.item_id());
            m_time = Time::invalid();
        }

        /// Sets the Property clean.
        /// @param time     Time when the decision to set the Property was made. Is `now` by default.
        void _set_clean(Time time = Time::invalid()) { m_time = (time.is_valid() ? std::move(time) : Time::now()); }

        // fields -------------------------------------------------------------
    private:
        /// Value held by the Property.
        std::any m_value;

        /// Expression evaluating to a new value for this Property.
        std::function<std::any(const PropertyManager&)> m_expression;

        /// All Properties that this one depends on.
        std::vector<PropertyKey> m_dependencies;

        /// Properties affected by this one through expressions.
        std::vector<PropertyKey> m_affected;

        /// Time when this Property was last defined.
        /// Is invalid if the Property is dirty (its expression needs to be evaluated).
        Time m_time = Time::now();
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
    void set_property(const PropertyKey key, std::function<T(const PropertyManager&)>&& expression,
                      std::vector<PropertyKey> dependencies)
    {
        static_assert(!std::is_same<T, void>::value, "Cannot define a Property with an expression returning `void`");
        std::lock_guard<std::mutex> lock(m_mutex);
        _detect_cycles(key, dependencies);
        _find_property(key).set_expression(key, *this, std::move(expression), std::move(dependencies));
    }

    /// Returns the set of Items that had at least one Property changed or dirtied since the last call.
    /// Note that the set also contains the parent Items of those that have been removed.
    std::unordered_set<ItemId> swap_dirty_items()
    {
        std::unordered_set<ItemId> result;
        result.swap(m_dirty_items);
        return result;
    }

private: // for Access<Item>
    /// Creates an new Property for a given Item and assigns its initial value (and type).
    /// @param key      Key of the new Property.
    /// @param value    Initial value of the Property, also used to determine its type.
    /// @throws lookup_error    If the key is invalid or already exists.
    template<typename T>
    void _add_property(PropertyKey key, T&& value)
    {
        if (!key.is_valid()) {
            notf_throw(lookup_error, "Cannot add Property with invalid key");
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_properties.find(key);
        if (it != m_properties.end()) {
            notf_throw(lookup_error, "Cannot add Property with duplicate key");
        }
        m_dirty_items.insert(key.item_id());
        m_properties.emplace(std::make_pair(std::move(key), Property(std::forward<T>(value))));
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the PropertyKey does not identify a Property in the graph, the call is silently ignored.
    void _delete_property(const PropertyKey key);

private:
    /// Property access with proper error reporting.
    /// @param key              Key of the Property to find.
    /// @throws lookup_error    If a property with the requested key does not exist in the graph.
    /// @returns                The requested Property.
    Property& _find_property(const PropertyKey key)
    {
        if (key.property_id().is_valid()) {
            const auto it = m_properties.find(key);
            if (it != m_properties.end()) {
                return it->second;
            }
        }
        _throw_notfound(key);
    }
    const Property& _find_property(const PropertyKey key) const
    {
        return const_cast<PropertyManager*>(this)->_find_property(key);
    }

    /// Call when a PropertyKey is not found.
    NOTF_NORETURN void _throw_notfound(const PropertyKey key);

    /// Detects potential cyclic dependencies in the graph before they are introduced.
    /// @throws cyclic_dependency_error     If the expression would introduce a cyclic dependency into the graph.
    void _detect_cycles(const PropertyKey key, const std::vector<PropertyKey>& dependencies);

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// All properties in the graph.
    std::unordered_map<PropertyKey, Property> m_properties;

    /// All Items that had at least one Property changed or dirtied since the last call to `swap_dirty_properties`.
    std::unordered_set<ItemId> m_dirty_items;

    /// Mutex guarding the graph.
    mutable std::mutex m_mutex;
};

// ===================================================================================================================//

template<>
class PropertyManager::Access<Item> {
    friend class Item;

    /// Constructor.
    /// @param context  PropertyManager to access.
    Access(PropertyManager& graph) : m_graph(graph) {}

    /// Creates an new Property for a given Item and assigns its initial value (and type).
    /// @param key      Key of the new Property.
    /// @param value    Initial value of the Property, also used to determine its type.
    /// @throws lookup_error    If the key is invalid or already exists.
    template<typename T>
    void add_property(PropertyKey key, T&& value)
    {
        return m_graph._add_property(std::move(key), std::forward<T>(value));
    }

    /// Removes a Property from the graph.
    /// All affected Properties will have their value set to their current value.
    /// If the PropertyKey does not identify a Property in the graph, the call is silently ignored.
    /// @param key  Key of the new Property.
    void delete_property(PropertyKey key) { m_graph._delete_property(key); }

    /// Explictly adds a dirty Item to the set of dirty Items.
    /// Useful when deleting an Item, in which case its parent is dirtied.
    void add_dirty_item(ItemId item_id) { m_graph.m_dirty_items.insert(std::move(item_id)); }

    /// The PropertyManager to access.
    PropertyManager& m_graph;
};

NOTF_CLOSE_NAMESPACE
