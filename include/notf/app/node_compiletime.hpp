#pragma once

#include "./node.hpp"

NOTF_OPEN_NAMESPACE

// helper =========================================================================================================== //

namespace detail {

template<class>
struct wrap_tuple_elements_in_shared_ptrs;
template<class... Ts>
struct wrap_tuple_elements_in_shared_ptrs<std::tuple<Ts...>> {
    using type = std::tuple<std::shared_ptr<Ts>...>;
};
template<class T>
using wrap_tuple_elements_in_shared_ptrs_t = typename wrap_tuple_elements_in_shared_ptrs<T>::type;

} // namespace detail

// compile time node ================================================================================================ //

template<class Policy>
class CompileTimeNode : public Node {

    // types ----------------------------------------------------------------------------------- //
protected:
    /// Tuple containing all compile time Properties of this Node type.
    using properties_t = detail::wrap_tuple_elements_in_shared_ptrs_t<typename Policy::properties>;

    /// Type of a specific Property in this Node type.
    template<size_t I>
    using property_t = typename std::tuple_element_t<I, properties_t>::element_type;

    /// Number of Properties in this Node type.
    static constexpr const size_t s_property_count = std::tuple_size_v<properties_t>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default Constructor.
    /// Takes no arguments since all of the customization happens through the Policy type.
    CompileTimeNode() : Node(this) { _initialize_properties(); }

    /// Use the base class' `get_property(std::string_view)` method alongside the compile time implementations below.
    using Node::get_property;

    /// Returns a correctly typed Handle to a CompileTimeProperty or void (which doesn't compile).
    /// @param name     Name of the requested Property.
    template<char... Cs>
    auto get_property(StringType<char, Cs...> name) const
    {
        return _get_ct_property<0>(name);
    }

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

private:
    /// Initializes all Property shared_ptrs with their policy's default values and -visiblity.
    template<size_t I = 0>
    void _initialize_properties()
    {
        // create the new property
        auto property_ptr = std::make_shared<property_t<I>>();
        std::get<I>(m_properties) = property_ptr;

        // subscribe to receive an update, whenever the property changes its value
        auto typed_property = std::static_pointer_cast<Property<typename property_t<I>::value_t>>(property_ptr);
        property_t<I>::template AccessFor<Node>::get_operator(typed_property)->subscribe(_get_property_observer());

        if constexpr (I + 1 < s_property_count) { _initialize_properties<I + 1>(); }
    }

    /// Access to a CompileTimeProperty through the hash of its name.
    /// Allows access with a string at run time.
    template<size_t I = 0>
    AnyPropertyPtr _get_property(const size_t hash_value) const
    {
        if (property_t<I>::get_const_name().get_hash() == hash_value) {
            return std::static_pointer_cast<AnyProperty>(std::get<I>(m_properties));
        }
        if constexpr (I + 1 >= s_property_count) { return {}; }
        else {
            return _get_property<I + 1>(hash_value);
        }
    }

    /// Direct access to a CompileTimeProperty through its name alone.
    template<size_t I, char... Cs>
    auto _get_ct_property(StringType<char, Cs...> name) const
    {
        if constexpr (property_t<I>::get_const_name() == name) {
            using handle_t = typename property_t<I>::value_t;
            return PropertyHandle(std::static_pointer_cast<Property<handle_t>>(std::get<I>(m_properties)));
        }
        else if constexpr (I + 1 >= s_property_count) {
            return; // returns void
        }
        else {
            return _get_ct_property<I + 1>(name);
        }
    }

    /// Calculates the combined hash value of each Property in order.
    template<size_t I = 0>
    void _calculate_hash(size_t& result) const
    {
        hash_combine(result, std::get<I>(m_properties)->get());
        if constexpr (I + 1 < s_property_count) { _calculate_hash<I + 1>(result); }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    properties_t m_properties;
};

NOTF_CLOSE_NAMESPACE
