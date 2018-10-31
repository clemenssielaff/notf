#pragma once

#include "notf/app/node.hpp"
#include "notf/app/property_compiletime.hpp"

NOTF_OPEN_NAMESPACE

// helper =========================================================================================================== //

namespace detail {

/// Produces a std::shared_ptr<CompileTimeProperty<Policy>> for each Policy in the given tuple.
template<class>
struct create_compile_time_property;
template<class... Ts>
struct create_compile_time_property<std::tuple<Ts...>> {
    using type = std::tuple<std::shared_ptr<CompileTimeProperty<Ts>>...>;
};
template<class T>
using create_compile_time_properties = typename create_compile_time_property<T>::type;

} // namespace detail

// compile time node ================================================================================================ //

template<class Properties>
class CompileTimeNode : public Node {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Tuple containing all Property policies of this Node type.
    using property_policies_t = Properties;

protected:
    /// Tuple of Property instance types managed by this Node type.
    using NodeProperties = detail::create_compile_time_properties<Properties>;

    /// Type of a specific Property in this Node type.
    template<size_t I>
    using node_property_t = typename std::tuple_element_t<I, NodeProperties>::element_type;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    CompileTimeNode(valid_ptr<Node*> parent) : Node(parent) { _initialize_properties(); }

public:
    /// Returns a correctly typed Handle to a CompileTimeProperty or void (which doesn't compile).
    /// @param name     Name of the requested Property.
    template<char... Cs>
    auto get_property(StringType<Cs...> name) const
    {
        return _get_ct_property<0>(name);
    }

    /// Use the base class' `get_property(std::string_view)` method alongside the compile time implementation.
    using Node::get_property;

protected:
    /// Implementation specific query of a Property.
    AnyPropertyPtr _get_property(const std::string& name) const override { return _get_property(hash_string(name)); }

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash() const override
    {
        size_t result = detail::version_hash();
        _calculate_hash(result);
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() const override { _clear_property_data<0>(); }

    /// Access to a CompileTimeProperty through the hash of its name.
    /// Allows access with a string at run time.
    template<size_t I = 0>
    AnyPropertyPtr _get_property(const size_t hash_value) const
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            if (node_property_t<I>::get_const_name().get_hash() == hash_value) {
                return std::static_pointer_cast<AnyProperty>(std::get<I>(m_node_properties));
            }
            else {
                return _get_property<I + 1>(hash_value);
            }
        }
        else {
            return {}; // no such property
        }
    }

    /// Direct access to a CompileTimeProperty through its name alone.
    template<size_t I, char... Cs>
    auto _get_ct_property(StringType<Cs...> name) const
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            if constexpr (node_property_t<I>::get_const_name() == name) {
                using value_t = typename node_property_t<I>::value_t;
                return PropertyHandle(std::static_pointer_cast<Property<value_t>>(std::get<I>(m_node_properties)));
            }
            else {
                return _get_ct_property<I + 1>(name);
            }
        }
    }

    /// Calculates the combined hash value of each Property in order.
    template<size_t I = 0>
    void _calculate_hash(size_t& result) const
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            hash_combine(result, std::get<I>(m_node_properties)->get());
            _calculate_hash<I + 1>(result);
        }
    }

    /// Clear modified Property data.
    template<size_t I = 0>
    void _clear_property_data() const
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            std::get<I>(m_node_properties)->clear_modified_data();
            _clear_property_data<I + 1>();
        }
    }

private:
    /// Initializes all Properties with their default values and -visiblity.
    template<size_t I = 0>
    void _initialize_properties()
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            // create the new property
            auto property_ptr = std::make_shared<node_property_t<I>>();
            std::get<I>(m_node_properties) = property_ptr;

            // subscribe to receive an update, whenever the property changes its value
            auto typed_property
                = std::static_pointer_cast<Property<typename node_property_t<I>::value_t>>(property_ptr);
            typed_property->get_operator()->subscribe(_get_property_observer());

            _initialize_properties<I + 1>();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    NodeProperties m_node_properties;
};

NOTF_CLOSE_NAMESPACE
