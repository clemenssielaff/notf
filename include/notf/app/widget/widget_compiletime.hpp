#pragma once

#include "notf/meta/stringtype.hpp"

#include "notf/common/bitset.hpp"
#include "notf/common/variant.hpp"

#include "notf/app/widget/widget.hpp"

NOTF_OPEN_NAMESPACE

// state identifier ================================================================================================= //

namespace detail {

struct StateIdentifier {

    /// Tests if a given user defined State type is compatible to another with a given Node type.
    template<class UserState, class NodeType>
    static constexpr auto is_compatible() {
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
    static constexpr bool check() {
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
    static NodeType& _get_node(PreviousState& state) {
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
    constexpr void _transition_into() {
        static_assert(NodeType::template is_transition_possible<UserState, NextState>(), "Undefined State switch");
        m_node.template transition_into<NextState>();
    }

    // fields --------------------------------------------------------------------------------- //
private:
    /// Node that owns this State.
    NodeType& m_node;
};

// compile time widget ============================================================================================== //

namespace detail {

template<class Policy>
class _CompileTimeWidget : public AnyWidget, public CompileTimeNode<Policy> {

    // types ----------------------------------------------------------------------------------- //
public:
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
    static constexpr std::enable_if_t<(I < Last), bool> _is_state_name_taken(StringType<Cs...> name) {
        if constexpr (state_t<I>::name == name) {
            return true;
        } else {
            return _is_state_name_taken<I + 1, Last>(name);
        }
    }
    template<size_t I, size_t Last, char... Cs>
    static constexpr std::enable_if_t<(I == Last), bool> _is_state_name_taken(StringType<Cs...>) // msvc workaround, use
                                                                                                 // constexpr when fixed
    {
        return false; // not taken
    }

    /// Used to validate the State machine upon instantiation.
    template<size_t I = 0>
    static constexpr std::enable_if_t<(I < std::variant_size_v<StateVariant>), bool> _validate_state_machine() {
        if constexpr (I == 0) {
            // States may be listed only once in the variant
            static_assert(has_variant_unique_types<StateVariant>(),
                          "A State variant must only contain unique State types");
        }

        // every entry in the variant must be a valid subclass of State
        detail::StateIdentifier::check<state_t<I>>();

        // State names must be unique
        static_assert(!_is_state_name_taken<0, I>(make_string_type<state_t<I>::name>()), "State names must be unique");

        return _validate_state_machine<I + 1>();
    }
    template<size_t I = 0> // msvc workaround
    static constexpr std::enable_if_t<(I == std::variant_size_v<StateVariant>), bool> _validate_state_machine() {
        return true;
    }
    static_assert(_validate_state_machine());

    /// Checks if this Widget has a State by the given name.
    template<char... Cs>
    static constexpr bool _has_state() {
        return _get_state_index<0>(StringType<Cs...>{}) != std::variant_size_v<StateVariant>;
    }

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Widget.
    _CompileTimeWidget(valid_ptr<Node*> parent)
        : CompileTimeNode<Policy>(parent)
        , m_state(std::in_place_type_t<state_t<0>>(), *static_cast<typename state_t<0>::node_t*>(this)) {}

public:
    // state machine ----------------------------------------------------------

    /// The name of the current State.
    std::string_view get_state_name() const noexcept final {
        const char* name = _get_state_name(m_state.index());
        NOTF_ASSERT(name);
        return name;
    }

    /// @{
    /// Checks if a transition from one to the other State is possible.
    template<class CurrentState, class NewState>
    static constexpr bool is_valid_transition() {
        return std::is_constructible_v<NewState, std::add_rvalue_reference_t<CurrentState>>;
    }
    bool is_valid_transition(const std::string& from, std::string& to) const final {
        return _is_valid_transition(_get_state_index(from), _get_state_index(to));
    }
    /// @}

    /// @{
    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, class = std::enable_if_t<is_one_of_variant<NewState, StateVariant>()>>
    void transition_into() {
        _transition_into<NewState>();
    }
    template<char... Cs, class = std::enable_if_t<_has_state<Cs...>()>>
    void transition_into(StringType<Cs...> name) {
        _transition_into<0>(name);
    }
    void transition_into(const std::string& state) final {
        const size_t target_index = _get_state_index(state);
        if (target_index == std::variant_size_v<StateVariant>) {
            NOTF_THROW(AnyWidget::BadTransitionError, "Node {} has no State called \"{}\"", //
                       this->get_uuid().to_string(), state);
        }

        if (!_is_valid_transition(m_state.index(), target_index)) {
            NOTF_THROW(AnyWidget::BadTransitionError,
                       "Cannot transition Node {} from State \"{}\" into State \"{}\'", //
                       this->get_uuid().to_string(), get_state_name(), state);
        }
        _transition_into(target_index);
    }
    /// @}

private:
    // state machine ----------------------------------------------------------

    /// Current State of this Widget.
    const StateVariant& _get_current_state() const { return m_state; }

    /// @{
    /// Returns the index of a given State identified by name.
    /// @returns    Index of the State or size of StateVariant on error.
    template<size_t I = 0>
    std::enable_if_t<(I < std::variant_size_v<StateVariant>), size_t> _get_state_index(const std::string& name) const {
        if (name.compare(state_t<I>::name.c_str()) == 0) {
            return I;
        } else {
            return _get_state_index<I + 1>(name);
        }
    }
    template<size_t I = 0>
    std::enable_if_t<(I == std::variant_size_v<StateVariant>), size_t> _get_state_index(const std::string&) const {
        return I;
    }
    template<size_t I, char... Cs>
    static constexpr std::enable_if_t<(I < std::variant_size_v<StateVariant>), size_t>
    _get_state_index(StringType<Cs...> name) {
        if constexpr (state_t<I>::name == name) {
            return I;
        } else {
            return _get_state_index<I + 1>(name);
        }
    }
    template<size_t I, char... Cs>
    static constexpr std::enable_if_t<(I == std::variant_size_v<StateVariant>), size_t>
    _get_state_index(StringType<Cs...>) {
        return I;
    }
    /// @}

    /// The name of a State by its index.
    /// @returns    Name of the given State, is empty on error.
    template<size_t I = 0>
    static constexpr std::enable_if_t<(I < std::variant_size_v<StateVariant>), const char*>
    _get_state_name(const size_t index) {
        if (I == index) {
            return state_t<I>::name.c_str();
        } else {
            return _get_state_name<I + 1>(index);
        }
    }
    template<size_t I = 0>
    static constexpr std::enable_if_t<(I == std::variant_size_v<StateVariant>), const char*>
    _get_state_name(const size_t) {
        return nullptr;
    }

    /// @{
    /// Checks if this Widget type can transition between two States identified by their index.
    template<size_t From, size_t To>
    static constexpr bool _check_transition(const size_t to) {
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
    static constexpr bool _check_transition(const size_t from, const size_t to) {
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
    bool _is_valid_transition(const size_t from, const size_t to) const {
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
    std::enable_if_t<(I < std::variant_size_v<StateVariant>)> _transition_into(StringType<Cs...> name) {
        if constexpr (state_t<I>::name == name) {
            return _transition_into<state_t<I>>();
        } else {
            return _transition_into<I + 1>(name);
        }
    }
    template<size_t I, char... Cs>
    std::enable_if_t<(I == std::variant_size_v<StateVariant>)> _transition_into(StringType<Cs...>) {
        static_assert(always_false_v<I>);
    }

    /// Transitions frm the current into another State identified by name.
    template<size_t I = 0>
    std::enable_if_t<(I < std::variant_size_v<StateVariant>)> _transition_into(const size_t index) {
        if (I == index) {
            return _transition_into<state_t<I>>();
        } else {
            return _transition_into<I + 1>(index);
        }
    }
    template<size_t I = 0>
    std::enable_if_t<(I == std::variant_size_v<StateVariant>)> _transition_into(const size_t) {
        NOTF_ASSERT(false);
    }

    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, std::size_t I = 0>
    std::enable_if_t<(I < std::variant_size_v<StateVariant>)> _transition_into() {
        if constexpr (is_valid_transition<state_t<I>, NewState>()) {
            if (m_state.index() == I) {
                m_state.template emplace<NewState>(std::move(std::get<I>(m_state)));
                return;
            }
        }
        return _transition_into<NewState, I + 1>();
    }
    template<class NewState, std::size_t I = 0>
    std::enable_if_t<(I == std::variant_size_v<StateVariant>)> _transition_into() {
        NOTF_THROW(AnyWidget::BadTransitionError,
                   "Cannot transition Node {} from State \"{}\" into State \"{}\'", //
                   this->get_uuid().to_string(), get_state_name(), NewState::name.c_str());
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Current State of this Widget.
    StateVariant m_state;
};

} // namespace detail

/// Convenience typdef hiding the fact that every Widget derives from `Widget` through CRTP.
template<class Policy>
using CompileTimeWidget = Widget<detail::_CompileTimeWidget<Policy>>;

NOTF_CLOSE_NAMESPACE
