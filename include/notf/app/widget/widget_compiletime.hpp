#pragma once

#include "notf/common/variant.hpp"

#include "notf/app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// state identifier ================================================================================================= //

namespace detail {

struct StateIdentifier {

    /// Tests if a given user defined State type is compatible to another with a given Node type.
    template<class UserState, class NodeType>
    static constexpr auto is_compatible()
    {
        if constexpr (std::disjunction_v<std::negation<decltype(has_this_t<UserState>(std::declval<UserState>()))>,
                                         std::negation<decltype(has_node_t<UserState>(std::declval<UserState>()))>>) {
            return std::false_type{}; // not a state
        } else {
            return std::conjunction<
                std::is_convertible<UserState*, State<typename UserState::this_t, typename UserState::node_t>*>,
                std::is_same<typename UserState::node_t, NodeType>>{};
        }
    }

    /// Checks a given user defined State type and static asserts with an error message if something is wrong.
    template<class UserState>
    static constexpr bool check()
    {
        static_assert(std::is_convertible_v<UserState*, State<typename UserState::this_t, typename UserState::node_t>*>,
                      "User State is not a valid subclass of State");

        static_assert(decltype(has_name<UserState>(std::declval<UserState>()))::value,
                      "State subclasses must have a unique name, stored as `static constexpr StringConst name`");
        static_assert(std::is_same_v<decltype(UserState::name), const StringConst>,
                      "State subclasses must have a unique name, stored as `static constexpr StringConst name`");
        static_assert(UserState::name.get_size() > 0, "State names must not be empty");

        return true;
    }

    /// Checks, whether the given type has a nested type `this_t`.
    template<class T>
    static constexpr auto has_this_t(const T&) -> decltype(std::declval<typename T::this_t>(), std::true_type{});
    template<class>
    static constexpr auto has_this_t(...) -> std::false_type;

    /// Checks, whether the given type has a nested type `node_t`.
    template<class T>
    static constexpr auto has_node_t(const T&) -> decltype(std::declval<typename T::node_t>(), std::true_type{});
    template<class>
    static constexpr auto has_node_t(...) -> std::false_type;

    /// Checks, whether the given type has a static field `name`.
    template<class T>
    static constexpr auto has_name(const T&) -> decltype(T::name, std::true_type{});
    template<class>
    static constexpr auto has_name(...) -> std::false_type;
};

} // namespace detail

// state ============================================================================================================ //

template<class UserState, class NodeType>
class State {

    template<class, class>
    friend class State; // befriend all other States so you can steal their node instance

    // types ----------------------------------------------------------------------------------- //
public:
    /// The derived State type.
    using this_t = UserState;

    /// Node type that this State is for.
    using node_t = NodeType;

protected:
    /// Base type for derived class.
    using base_t = State<UserState, NodeType>;

    /// Whether T is a State compatible to this one.
    template<class T>
    using is_compatible_state = decltype(detail::StateIdentifier::is_compatible<T, node_t>());

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value Constructor.
    /// @param node     Node that owns this State.
    State(NodeType& node) : m_node(node) {}

protected:
    /// Get a compatible Node from any other State.
    template<class PreviousState>
    static NodeType& _get_node(PreviousState& state)
    {
        static_assert(is_compatible_state<PreviousState>::value,
                      "States are incompatible because they have different Node types");
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
    template<class NextState>
    constexpr void _transition_into()
    {
        static_assert(NodeType::template is_transition_possible<UserState, NextState>(), "Undefined State switch");
        m_node.template transition_into<NextState>();
    }

    // fields --------------------------------------------------------------------------------- //
private:
    /// Node that owns this State.
    NodeType& m_node;
};

// compile time widget ============================================================================================== //

template<class Policy>
class CompileTimeWidget : public AnyWidget {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Tuple containing all Property policies of this Node type.
    using property_policies_t = concat_tuple_t<AnyWidget::property_policies_t, typename Policy::properties>;

public:
    /// Tuple of Property instance types managed by this Widget type.
    using WidgetProperties = detail::create_compile_time_properties<typename Policy::properties>;

    /// Type of a specific Property in this Widget type.
    template<size_t I>
    using widget_property_t = typename std::tuple_element_t<I, WidgetProperties>::element_type;

    /// Variant of all States of this Widget type.
    using StateVariant = typename Policy::states;

    /// Type of a specific State of this Widget.
    template<size_t I>
    using state_t = std::variant_alternative_t<I, StateVariant>;

    /// Matrix indicating which State transitions are possible.
    using TransitionTable = std::bitset<std::variant_size_v<StateVariant> * std::variant_size_v<StateVariant>>;

    // helper ---------------------------------------------------------------------------------- //
private:
    /// Checks if a given name is already in use in a sub-range of all States.
    template<size_t I, size_t Last, char... Cs>
    static constexpr bool _is_state_name_taken(StringType<Cs...> name)
    {
        if constexpr (I == Last) {
            return false;
        } else if constexpr (state_t<I>::name == name) {
            return true;
        } else {
            return _is_state_name_taken<I + 1, Last>(name);
        }
    }

    /// Used to validate the State machine upon instantiation.
    template<size_t I = 0>
    static constexpr bool _validate_state_machine()
    {
        if constexpr (I == std::variant_size_v<StateVariant>) {
            return true;
        } else {
            if constexpr (I == 0) {
                // States may be listed only once in the variant
                static_assert(has_variant_unique_types<StateVariant>(),
                              "A State variant must only contain unique State types");
            }

            // every entry in the variant must be a valid subclass of State
            detail::StateIdentifier::check<state_t<I>>();

            // State names must be unique
            static_assert(!_is_state_name_taken<0, I>(make_string_type<state_t<I>::name>()),
                          "State names must be unique");

            return _validate_state_machine<I + 1>();
        }
    }
    static_assert(_validate_state_machine());

    /// Checks if this Widget has a State by the given name.
    template<char... Cs>
    static constexpr bool _has_state()
    {
        return _get_state_index<0>(StringType<Cs...>{}) != std::variant_size_v<StateVariant>;
    }

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Widget.
    CompileTimeWidget(valid_ptr<Node*> parent)
        : AnyWidget(parent)
        , m_state(std::in_place_type_t<state_t<0>>(), *static_cast<typename state_t<0>::node_t*>(this))
    {
        _initialize_properties();
    }

public:
    // state machine ----------------------------------------------------------

    /// The name of the current State.
    std::string_view get_state_name() const noexcept override
    {
        const char* name = _get_state_name(m_state.index());
        NOTF_ASSERT(name);
        return name;
    }

    /// @{
    /// Checks if a transition from one to the other State is possible.
    template<class CurrentState, class NewState>
    static constexpr bool is_valid_transition()
    {
        return std::is_constructible_v<NewState, std::add_rvalue_reference_t<CurrentState>>;
    }
    bool is_valid_transition(const std::string& from, std::string& to) const override
    {
        return _is_valid_transition(_get_state_index(from), _get_state_index(to));
    }
    /// @}

    /// @{
    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, class = std::enable_if_t<is_one_of_variant<NewState, StateVariant>()>>
    void transition_into()
    {
        _transition_into<NewState>();
    }
    template<char... Cs, class = std::enable_if_t<_has_state<Cs...>()>>
    void transition_into(StringType<Cs...> name)
    {
        _transition_into<0>(name);
    }
    void transition_into(const std::string& state) override
    {
        const size_t target_index = _get_state_index(state);
        if (target_index == std::variant_size_v<StateVariant>) {
            NOTF_THROW(AnyWidget::BadTransitionError, "Node {} has no State called \"{}\"", //
                       get_uuid().to_string(), state);
        }

        if (!_is_valid_transition(m_state.index(), target_index)) {
            NOTF_THROW(AnyWidget::BadTransitionError,
                       "Cannot transition Node {} from State \"{}\" into State \"{}\'", //
                       get_uuid().to_string(), get_state_name(), state);
        }
        _transition_into(target_index);
    }
    /// @}

    // properties ----------------------------------------------------------

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
    // state machine ----------------------------------------------------------

    /// Current State of this Widget.
    const StateVariant& _get_current_state() const { return m_state; }

    /// @{
    /// Returns the index of a given State identified by name.
    /// @returns    Index of the State or size of StateVariant on error.
    template<size_t I = 0>
    size_t _get_state_index(const std::string& name) const
    {
        if constexpr (I == std::variant_size_v<StateVariant>) {
            return I;
        } else {
            if (name.compare(state_t<I>::name.c_str()) == 0) {
                return I;
            } else {
                return _get_state_index<I + 1>(name);
            }
        }
    }
    template<size_t I, char... Cs>
    static constexpr size_t _get_state_index(StringType<Cs...> name)
    {
        if constexpr (I == std::variant_size_v<StateVariant> || state_t<I>::name == name) {
            return I;
        } else {
            return _get_state_index<I + 1>(name);
        }
    }
    /// @}

    /// The name of a State by its index.
    /// @returns    Name of the given State, is empty on error.
    template<size_t I = 0>
    static constexpr const char* _get_state_name(const size_t index)
    {
        if constexpr (I == std::variant_size_v<StateVariant>) {
            return nullptr;
        } else {
            if (I == index) {
                return state_t<I>::name.c_str();
            } else {
                return _get_state_name<I + 1>(index);
            }
        }
    }

    /// @{
    /// Checks if this Widget type can transition between two States identified by their index.
    template<size_t From, size_t To>
    static constexpr bool _check_transition(const size_t to)
    {
        if constexpr (To == std::variant_size_v<StateVariant>) {
            return false;
        } else {
            if (To == to) {
                return is_valid_transition<state_t<From>, state_t<To>>();
            } else {
                return _check_transition<From, To + 1>(to);
            }
        }
    }
    template<size_t From>
    static constexpr bool _check_transition(const size_t from, const size_t to)
    {
        if constexpr (From == std::variant_size_v<StateVariant>) {
            return false;
        } else {
            if (From == from) {
                return _check_transition<From, 0>(to);
            } else {
                return _check_transition<From + 1>(from, to);
            }
        }
    }
    /// #}

    /// Checks if a transition from one to the other State is possible.
    bool _is_valid_transition(const size_t from, const size_t to) const
    {
        constexpr auto to_table_index = [](const size_t from, const size_t to) {
            NOTF_ASSERT(from < std::variant_size_v<StateVariant>);
            NOTF_ASSERT(to < std::variant_size_v<StateVariant>);
            return from + (std::variant_size_v<StateVariant> * to);
        };

        static const TransitionTable table = [&]() {
            TransitionTable result;
            for (size_t from = 0; from < std::variant_size_v<StateVariant>; ++from) {
                for (size_t to = 0; to < std::variant_size_v<StateVariant>; ++to) {
                    result[to_table_index(from, to)] = _check_transition<0>(from, to);
                }
            }
            return result;
        }();

        if (from == std::variant_size_v<StateVariant> || to == std::variant_size_v<StateVariant>) { return false; }

        return table[to_table_index(from, to)];
    }

    /// Transitions frm the current into another State identified by name.
    template<size_t I, char... Cs>
    void _transition_into(StringType<Cs...> name)
    {
        static_assert(I < std::variant_size_v<StateVariant>);
        if constexpr (state_t<I>::name == name) {
            return _transition_into<state_t<I>>();
        } else {
            return _transition_into<I + 1>(name);
        }
    }

    /// Transitions frm the current into another State identified by name.
    template<size_t I = 0>
    void _transition_into(const size_t index)
    {
        if constexpr (I == std::variant_size_v<StateVariant>) {
            NOTF_ASSERT(false);
        } else {
            if (I == index) {
                return _transition_into<state_t<I>>();
            } else {
                return _transition_into<I + 1>(index);
            }
        }
    }

    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, std::size_t I = 0>
    void _transition_into()
    {
        if constexpr (I >= std::variant_size_v<StateVariant>) {
            NOTF_THROW(AnyWidget::BadTransitionError,
                       "Cannot transition Node {} from State \"{}\" into State \"{}\'", //
                       get_uuid().to_string(), get_state_name(), NewState::name.c_str());
        } else {
            if constexpr (is_valid_transition<state_t<I>, NewState>()) {
                if (m_state.index() == I) {
                    m_state.template emplace<NewState>(std::move(std::get<I>(m_state)));
                    return;
                }
            }
            return _transition_into<NewState, I + 1>();
        }
    }

    // properties ----------------------------------------------------------

    // TODO: it should be possible to create member templates in CompileTimeNode so CompileTimeWidget can call templates
    // instead of copying most of the code

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

    /// Initializes all Properties with their default values and -visiblity.
    template<size_t I = 0>
    void _initialize_properties()
    {
        if constexpr (I < std::tuple_size_v<WidgetProperties>) {
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
        if constexpr (I < std::tuple_size_v<WidgetProperties>) {
            if (widget_property_t<I>::get_const_name().get_hash() == hash_value) {
                return std::static_pointer_cast<AnyProperty>(std::get<I>(m_widget_properties));
            } else {
                return _get_property<I + 1>(hash_value);
            }
        } else {
            return AnyWidget::_get_property(hash_value);
        }
    }

    /// Direct access to a CompileTimeProperty through its name alone.
    template<size_t I, char... Cs>
    auto _get_ct_property(StringType<Cs...> name) const
    {
        if constexpr (I < std::tuple_size_v<WidgetProperties>) {
            if constexpr (widget_property_t<I>::get_const_name() == name) {
                using value_t = typename widget_property_t<I>::value_t;
                return PropertyHandle(std::static_pointer_cast<Property<value_t>>(std::get<I>(m_widget_properties)));
            } else {
                return _get_ct_property<I + 1>(name);
            }
        } else {
            return AnyWidget::_get_ct_property(name);
        }
    }

    /// Calculates the combined hash value of each Property in order.
    template<size_t I = 0>
    void _calculate_hash(size_t& result) const
    {
        if constexpr (I < std::tuple_size_v<WidgetProperties>) {
            hash_combine(result, std::get<I>(m_widget_properties)->get());
            _calculate_hash<I + 1>(result);
        } else {
            return AnyWidget::_calculate_hash(result);
        }
    }

    /// Clear modified Property data.
    template<size_t I = 0>
    void _clear_property_data() const
    {
        if constexpr (I < std::tuple_size_v<WidgetProperties>) {
            std::get<I>(m_widget_properties)->clear_modified_data();
            _clear_property_data<I + 1>();
        } else {
            return AnyWidget::_clear_property_data();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// All Properties of this Widget, default initialized to the Definition's default values.
    WidgetProperties m_widget_properties;

    /// Current State of this Widget.
    StateVariant m_state;
};

NOTF_CLOSE_NAMESPACE
