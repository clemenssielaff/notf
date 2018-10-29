#pragma once

#include "notf/common/variant.hpp"

#include "notf/app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// state ============================================================================================================ //

template<template<class> class Derived, class NodeType>
class State {

    template<template<class> class, class>
    friend class State; // befriend all other States of the Node Type, so you can steal their nodes

    // types ----------------------------------------------------------------------------------- //
protected:
    /// Base type for derived class.
    using base_t = State<Derived, NodeType>;

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value Constructor.
    /// @param node     Node that ownes this State.
    State(NodeType& node) : m_node(node) {}

    /// Get a compatible Node from any other State.
    template<template<class> class Other>
    static NodeType& _get_node(State<Other, NodeType>& state)
    {
        return state.m_node;
    }

public:
    /// forbid the move constructor, otherwise all states would always be able to transition to themselves
    /// (can be overridden in subclasses)
    State(State&&) = delete;

    /// Virtual Destructor.
    virtual ~State() = default;

protected:
    /// Transition into another State.
    template<template<class> class T>
    constexpr void _transition_into()
    {
        static_assert(NodeType::template can_transition<Derived<NodeType>, T<NodeType>>(), "Undefined State switch");
        m_node.template transition_into<T<NodeType>>();
    }

    // fields --------------------------------------------------------------------------------- //
private:
    /// Node that ownes this State.
    NodeType& m_node;
};

// compile time widget ============================================================================================== //

template<class Policy>
class CompileTimeWidget : public AnyWidget {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Tuple containing all Property policies of this Node type.
    using property_policies_t = concat_tuple_t<AnyWidget::property_policies_t, typename Policy::properties>;

public:
    /// Tuple of Property instance types managed by this Widget type.
    using widget_properties_t = detail::create_compile_time_property_t<typename Policy::properties>;

    /// Type of a specific Property in this Widget type.
    template<size_t I>
    using widget_property_t = typename std::tuple_element_t<I, widget_properties_t>::element_type;

    /// Number of Properties in this Node type.
    static constexpr size_t s_widget_property_count = std::tuple_size_v<widget_properties_t>;

    /// Variant of all States of this Widget type.
    using state_variant_t = typename Policy::states;

    /// Type of a specific State of thiw Widget.
    template<size_t I>
    using state_t = std::variant_alternative_t<I, state_variant_t>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    CompileTimeWidget(valid_ptr<Node*> parent) : AnyWidget(parent), m_state(std::in_place_type_t<state_t<0>>(), *this)
    {
        AnyWidget::_initialize_properties();
        _initialize_properties();
    }

public:
    /// Returns a correctly typed Handle to a CompileTimeProperty or void (which doesn't compile).
    /// @param name     Name of the requested Property.
    template<char... Cs>
    auto get_property(StringType<char, Cs...> name) const
    {
        return _get_ct_property<0>(name);
    }

    /// Use the base class' `get_property(std::string_view)` method alongside the compile time implementation.
    using Node::get_property;

    /// Checks if a transition from one to the other State is possible.
    template<class CurrentState, class NewState>
    static constexpr bool can_transition()
    {
        return std::is_constructible_v<NewState, std::add_rvalue_reference_t<CurrentState>>;
    }

    /// @{
    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, class = std::enable_if_t<is_one_of_variant<NewState, state_variant_t>()>>
    void transition_into()
    {
        _transition_into<NewState>();
    }
    void transition_into(const size_t state_index) { _transition_into_index(state_index); }
    /// @}

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

    /// Current State of this Widget.
    const state_variant_t& _get_state() const { return m_state; }

    /// Initializes all Properties with their default values and -visiblity.
    template<size_t I = 0>
    void _initialize_properties()
    {
        if constexpr (I < s_widget_property_count) {
            // create the new property
            auto property_ptr = std::make_shared<widget_property_t<I>>();
            std::get<I>(m_widget_properties) = property_ptr;

            // subscribe to receive an update, whenever the property changes its value
            auto typed_property
                = std::static_pointer_cast<Property<typename widget_property_t<I>::value_t>>(property_ptr);
            typed_property->get_operator()->subscribe(_get_property_observer());

            _initialize_properties<I + 1>();
        }
    }

    /// Access to a CompileTimeProperty through the hash of its name.
    /// Allows access with a string at run time.
    template<size_t I = 0>
    AnyPropertyPtr _get_property(const size_t hash_value) const
    {
        if constexpr (I < s_widget_property_count) {
            if (widget_property_t<I>::get_const_name().get_hash() == hash_value) {
                return std::static_pointer_cast<AnyProperty>(std::get<I>(m_widget_properties));
            }
            else {
                return _get_property<I + 1>(hash_value);
            }
        }
        else {
            return AnyWidget::_get_property(hash_value);
        }
    }

    /// Direct access to a CompileTimeProperty through its name alone.
    template<size_t I, char... Cs>
    auto _get_ct_property(StringType<char, Cs...> name) const
    {
        if constexpr (I < s_widget_property_count) {
            if constexpr (widget_property_t<I>::get_const_name() == name) {
                using value_t = typename widget_property_t<I>::value_t;
                return PropertyHandle(std::static_pointer_cast<Property<value_t>>(std::get<I>(m_widget_properties)));
            }
            else {
                return _get_ct_property<I + 1>(name);
            }
        }
        else {
            return AnyWidget::_get_ct_property(name);
        }
    }

    /// Calculates the combined hash value of each Property in order.
    template<size_t I = 0>
    void _calculate_hash(size_t& result) const
    {
        if constexpr (I < s_widget_property_count) {
            hash_combine(result, std::get<I>(m_widget_properties)->get());
            _calculate_hash<I + 1>(result);
        }
        else {
            return AnyWidget::_calculate_hash(result);
        }
    }

    /// Clear modified Property data.
    template<size_t I = 0>
    void _clear_property_data() const
    {
        if constexpr (I < s_widget_property_count) {
            std::get<I>(m_widget_properties)->clear_modified_data();
            _clear_property_data<I + 1>();
        }
        else {
            return AnyWidget::_clear_property_data();
        }
    }

    /// Transitions frm the current into another State identified by index.
    template<size_t I = 0>
    void _transition_into_index(const size_t state_index)
    {
        if constexpr (I < std::variant_size_v<state_variant_t>) {
            if (I == state_index) { return _transition_into<state_t<I>>(); }
            else {
                return _transition_into_index<I + 1>(state_index);
            }
        }
        else {
            NOTF_THROW(AnyWidget::BadTransitionError,
                       "Cannot transition Node {} from State index {} into State index {}", get_uuid().to_string(),
                       m_state.index(), state_index);
        }
    }

    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, std::size_t I = 0>
    void _transition_into()
    {
        if constexpr (I < std::variant_size_v<state_variant_t>) {
            if constexpr (can_transition<state_t<I>, NewState>()) {
                if (m_state.index() == I) {
                    m_state.template emplace<NewState>(std::move(std::get<I>(m_state)));
                    return;
                }
            }
            return _transition_into<NewState, I + 1>();
        }
        else {
            NOTF_THROW(AnyWidget::BadTransitionError,
                       "Cannot transition Node {} from State index {} into State index {}", //
                       get_uuid().to_string(), m_state.index(), get_first_variant_index<NewState, state_variant_t>());
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Widget, default initialized to the Definition's default values.
    widget_properties_t m_widget_properties;

    /// Current State of this Widget.
    state_variant_t m_state;
};

NOTF_CLOSE_NAMESPACE
