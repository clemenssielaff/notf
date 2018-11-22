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

template<class Policy>
struct NodePolicyFactory {

    // TODO: (NodePolicyFactory) check that all names are unique

    /// Factory method.
    static constexpr auto create()
    {
        struct NodePolicy {

            /// All Properties of the Node type.
            using properties = create_compile_time_properties<decltype(get_properties())>;

            /// All Slots of the Node type.
            using slots = decltype(get_slots());

            /// All Signals of the Node type.
            using signals = decltype(get_signals());
        };

        return NodePolicy();
    }

    /// Checks, whether the given type has a nested type `properties`.
    template<class T>
    static constexpr auto _has_properties(const T&)
        -> decltype(std::declval<typename T::properties>(), std::true_type{});
    template<class>
    static constexpr auto _has_properties(...) -> std::false_type;
    static constexpr auto get_properties() noexcept
    {
        if constexpr (decltype(_has_properties<Policy>(std::declval<Policy>()))::value) {
            return std::declval<typename Policy::properties>();
        } else {
            return std::tuple<>{};
        }
    }

    template<class T>
    static constexpr auto _has_slots(const T&) -> decltype(std::declval<typename T::slots>(), std::true_type{});
    template<class>
    static constexpr auto _has_slots(...) -> std::false_type;
    static constexpr auto get_slots() noexcept
    {
        if constexpr (decltype(_has_slots<Policy>(std::declval<Policy>()))::value) {
            return std::declval<typename Policy::slots>();
        } else {
            return std::tuple<>{};
        }
    }

    template<class T>
    static constexpr auto _has_signals(const T&) -> decltype(std::declval<typename T::signals>(), std::true_type{});
    template<class>
    static constexpr auto _has_signals(...) -> std::false_type;
    static constexpr auto get_signals() noexcept
    {
        if constexpr (decltype(_has_signals<Policy>(std::declval<Policy>()))::value) {
            return std::declval<typename Policy::signals>();
        } else {
            return std::tuple<>{};
        }
    }
};

struct EmptyNodePolicy {
    using properties = std::tuple<>;
};

} // namespace detail

// compile time node ================================================================================================ //

template<class Policy = detail::EmptyNodePolicy>
class CompileTimeNode : public Node {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Node type.
    using policy_t = decltype(detail::NodePolicyFactory<Policy>::create());

protected:
    /// Tuple of Property instance types managed by this Node type.
    using Properties = typename policy_t::properties;

    /// Type of a specific Property in this Node type.
    template<size_t I>
    using property_t = typename std::tuple_element_t<I, Properties>::element_type;

    // helper --------------------------------------------------------------------------------- //
private:
    /// Finds the index of a Property by its name.
    template<size_t I = 0, char... Cs>
    static constexpr size_t _get_property_index(StringType<Cs...> name)
    {
        if constexpr (I < std::tuple_size_v<Properties>) {
            if constexpr (property_t<I>::get_const_name() == name) {
                return I;
            } else {
                return _get_property_index<I + 1>(name);
            }
        } else {
            NOTF_THROW(OutOfBounds);
        }
    }

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
    template<char... Cs, size_t I = _get_property_index(StringType<Cs...>{})>
    constexpr auto get_property(StringType<Cs...>)
    {
        return PropertyHandle<typename property_t<I>::value_t>(std::get<I>(m_node_properties));
    }
    template<const StringConst& name>
    constexpr auto get_property()
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
    template<char... Cs, class T, size_t I = _get_property_index(StringType<Cs...>{})>
    constexpr void set(StringType<Cs...>, T&& value)
    {
        if constexpr (std::is_convertible_v<T, property_t<I>>) {
            std::get<I>(m_node_properties)->set(std::forward<T>(value));
        }
    }
    template<const StringConst& name, class T>
    constexpr void set(T&& value)
    {
        set(make_string_type<name>(), std::forward<T>(value));
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using Node::get;
    using Node::get_property;
    using Node::set;

protected:
    /// @{
    /// (Re-)Defines a callback to be invoked every time the value of the Property is about to change.
    template<char... Cs, size_t I = _get_property_index(StringType<Cs...>{})>
    void _set_property_callback(StringType<Cs...>, typename property_t<I>::callback_t callback)
    {
        std::get<I>(m_node_properties)->set_callback(std::move(callback));
    }
    template<const StringConst& name, size_t I = _get_property_index(make_string_type<name>())>
    constexpr void _set_property_callback(typename property_t<I>::callback_t callback)
    {
        _set_property_callback(make_string_type<name>(), std::move(callback));
    }
    /// @}

    // Use the base class' runtime methods alongside the compile time implementation.
    using Node::_set_property_callback;

private:
    /// @{
    /// Implementation specific query of a Property.
    AnyPropertyPtr _get_property(const std::string& name) final { return _get_property_impl(hash_string(name)); }
    template<size_t I = 0>
    AnyPropertyPtr _get_property_impl(const size_t hash_value)
    {
        if constexpr (I < std::tuple_size_v<Properties>) {
            if (property_t<I>::get_const_name().get_hash() == hash_value) {
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
    size_t _calculate_property_hash(size_t result = detail::version_hash()) const final
    {
        for_each(m_node_properties, [](auto& property, size_t& out) { hash_combine(out, property->get()); }, result);
        return result;
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() final
    {
        for_each(m_node_properties, [](auto& property) { property->clear_modified_data(); });
    }

    /// Finalizes this Node.
    /// Stores a Handle of this node in all of its Properties.
    void _finalize() final
    {
        for_each(m_node_properties, [this](auto& property) { //
            AnyProperty::AccessFor<Node>::set_node(*property, shared_from_this());
        });
        Node::_finalize();
    }

    // hide some protected methods
    using Node::_finalize;
    using Node::_get_property_observer;
    using Node::_is_finalized;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    Properties m_node_properties;
};

NOTF_CLOSE_NAMESPACE
