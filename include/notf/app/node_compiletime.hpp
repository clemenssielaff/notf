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
    CompileTimeNode(valid_ptr<Node*> parent) : Node(parent)
    {
        for_each(m_node_properties, [this](auto& property) {
            using property_t = typename std::decay_t<decltype(property)>::element_type;

            // create the new property
            auto property_ptr = std::make_shared<property_t>();
            property = property_ptr;

            // subscribe to receive an update, whenever the property changes its value
            auto typed_property = std::static_pointer_cast<Property<typename property_t::value_t>>(property_ptr);
            typed_property->get_operator()->subscribe(_get_property_observer());
        });
    }

public:
    /// @{
    /// Returns a correctly typed Handle to a CompileTimeProperty or void (which doesn't compile).
    /// @param name     Name of the requested Property.
    template<char... Cs>
    constexpr auto get_property(StringType<Cs...> name) const
    {
        constexpr size_t index = _get_property_index(name);
        using value_t = typename node_property_t<index>::value_t;
        return PropertyHandle(std::static_pointer_cast<Property<value_t>>(std::get<index>(m_node_properties)));
    }
    template<const StringConst& name>
    constexpr auto get_property() const
    {
        return get_property(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Returns the correctly typed value of a CompileTimeProperty.
    /// @param name     Name of the requested Property.
    template<char... Cs>
    const auto& get(StringType<Cs...> name) const
    {
        return std::get<_get_property_index(name)>(m_node_properties)->get();
    }
    template<const StringConst& name>
    constexpr const auto& get() const
    {
        return get(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Updates the value of a CompileTimeProperty of this Node.
    /// @param name     Node-unique name of the Property.
    /// @param value    New value of the Property.
    template<char... Cs, class T>
    constexpr void set(StringType<Cs...> name, T&& value)
    {
        constexpr size_t index = _get_property_index(name);
        if constexpr (std::is_convertible_v<T, node_property_t<index>>) {
            std::get<index>(m_node_properties)->set(std::forward<T>(value));
        }
    }
    template<const StringConst& name, class T>
    constexpr void set(T&& value)
    {
        set(make_string_type<name>(), std::forward<T>(value));
    }
    /// @}

    /// Use the base class' runtime methods alongside the compile time implementation.
    using Node::get;
    using Node::get_property;
    using Node::set;

protected:
    /// @{
    /// Implementation specific query of a Property.
    AnyPropertyPtr _get_property(const std::string& name) const override
    {
        return _get_property_impl(hash_string(name));
    }
    template<size_t I = 0>
    AnyPropertyPtr _get_property_impl(const size_t hash_value) const
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            if (node_property_t<I>::get_const_name().get_hash() == hash_value) {
                return std::static_pointer_cast<AnyProperty>(std::get<I>(m_node_properties));
            } else {
                return _get_property_impl<I + 1>(hash_value);
            }
        } else {
            return {}; // no such property
        }
    }
    /// @}

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash(size_t result = detail::version_hash()) const override
    {
        for_each(m_node_properties, [](const auto& property, size_t& result) { hash_combine(result, property->get()); },
                 result);
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() override
    {
        for_each(m_node_properties, [](auto& property) { property->clear_modified_data(); });
    }

    /// @{
    /// Access to a CompileTimeProperty by name.
    template<char... Cs>
    auto _get_property(StringType<Cs...> name) const
    {
        return std::get<_get_property_index(name)>(m_node_properties);
    }
    template<const StringConst& name>
    constexpr auto _get_property() const
    {
        return _get_property(make_string_type<name>());
    }
    /// @}

    /// Finds the index of a Property by its name.
    template<size_t I = 0, char... Cs>
    static constexpr size_t _get_property_index(StringType<Cs...> name)
    {
        if constexpr (I < std::tuple_size_v<NodeProperties>) {
            if constexpr (node_property_t<I>::get_const_name() == name) {
                return I;
            } else {
                return _get_property_index<I + 1>(name);
            }
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    NodeProperties m_node_properties;
};

NOTF_CLOSE_NAMESPACE
