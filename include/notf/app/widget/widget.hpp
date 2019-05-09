#pragma once

#include "notf/common/variant.hpp"

#include "notf/app/widget/any_widget.hpp"

NOTF_OPEN_NAMESPACE

// widget policy factory ============================================================================================ //

namespace detail {

template<class Policy>
class WidgetPolicyFactory { // TODO: check that all names are unique and don't conflict with widget's

    NOTF_CREATE_TYPE_DETECTOR(properties);
    static constexpr auto create_properties() {
        if constexpr (_has_properties_v<Policy>) {
            return declval<typename instantiate_unique<Property, typename Policy::properties>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(slots);
    static constexpr auto create_slots() {
        if constexpr (_has_slots_v<Policy>) {
            return declval<typename instantiate_unique<Slot, typename Policy::slots>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(signals);
    static constexpr auto create_signals() {
        if constexpr (_has_signals_v<Policy>) {
            return declval<typename instantiate_shared<Signal, typename Policy::signals>::type>();
        } else {
            return std::tuple<>();
        }
    }

    NOTF_CREATE_TYPE_DETECTOR(states);
    static_assert(_has_states_v<Policy>, "Widgets require at least one State");

public:
    /// Policy Type.
    struct WidgetPolicy {

        /// All Properties of the Widget type.
        using properties = decltype(create_properties());

        /// All Slots of the Widget type.
        using slots = decltype(create_slots());

        /// All Signals of the Widget type.
        using signals = decltype(create_signals());

        /// All States of the Widget's state machine.
        using states = typename Policy::states;
    };
};

struct StateIdentifier {

    NOTF_CREATE_TYPE_DETECTOR(this_t);
    NOTF_CREATE_TYPE_DETECTOR(node_t);
    NOTF_CREATE_FIELD_DETECTOR(name);

    /// Tests if a given user defined State type is compatible to another with a given Node type.
    template<class UserState, class NodeType>
    static constexpr auto is_compatible() {
        if constexpr (!_has_this_t<UserState>() || !_has_node_t<UserState>()) {
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

        static_assert(_has_name<UserState>(),
                      "State subclasses must have a unique name, stored as `static constexpr ConstString name`");
        static_assert(std::is_same_v<decltype(UserState::name), const ConstString>,
                      "State subclasses must have a unique name, stored as `static constexpr ConstString name`");
        static_assert(UserState::name.get_size() > 0, "State names must not be empty");

        return true;
    }
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
    using super_t = State<UserState, NodeType>;

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
    /// Forbid the move constructor, otherwise all states would always be able to transition to themselves
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

// widget =========================================================================================================== //

template<class Policy = detail::EmptyNodePolicy>
class Widget : public AnyWidget {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy used to create this Widget type.
    using widget_policy_t = typename detail::WidgetPolicyFactory<Policy>::WidgetPolicy;

private:
    /// Variant of all States of this Widget type.
    using StateVariant = typename Policy::states;
    using StateTuple = variant_to_tuple_t<StateVariant>;

    /// Type of a specific State of this Widget.
    template<size_t I>
    using state_t = std::variant_alternative_t<I, StateVariant>;

    /// Matrix indicating which State transitions are possible.
    using TransitionTable = std::bitset<std::variant_size_v<StateVariant> * std::variant_size_v<StateVariant>>;

    /// Tuple of Property types managed by this Widget.
    using WidgetProperties = typename widget_policy_t::properties;

    /// Type of a specific Property of this Widget.
    template<size_t I>
    using widget_property_t = typename std::tuple_element_t<I, WidgetProperties>::element_type;

    /// Tuple of Slot types managed by this Widget.
    using WidgetSlots = typename widget_policy_t::slots;

    /// Type of a specific Slot of this Widget.
    template<size_t I>
    using widget_slot_t = typename std::tuple_element_t<I, WidgetSlots>::element_type;

    /// Tuple of Signals types managed by this Widget.
    using WidgetSignals = typename widget_policy_t::signals;

    /// Type of a specific Signal of this Widget.
    template<size_t I>
    using widget_signal_t = typename std::tuple_element_t<I, WidgetSignals>::element_type;

    // helper ---------------------------------------------------------------------------------- //
private:
    /// Validates the State machine upon instantiation.
    template<size_t I = 0>
    static constexpr bool _validate_state_machine() {
        if constexpr (I < std::variant_size_v<StateVariant>) {
            if constexpr (I == 0) {
                // States may be listed only once in the variant
                static_assert(has_variant_unique_types<StateVariant>(),
                              "A State variant must only contain unique State types");
            }

            // every entry in the variant must be a valid subclass of State
            detail::StateIdentifier::check<state_t<I>>();

            // State names must be unique
            static_assert(_get_index<StateTuple>(make_string_type<state_t<I>::name>()) == I,
                          "State names must be unique");

            return _validate_state_machine<I + 1>();
        } else {
            return true;
        }
    }
    static_assert(_validate_state_machine());

    // clang-format off
    /// Helper struct to identify a Property index, type and whether it is a Widget- or a Node Property.
    template<char... Cs>
    struct _PropertyStudy {
        inline static constexpr StringType name = StringType<Cs...>{};
        inline static constexpr bool is_widget = _get_index<WidgetProperties>(name) < std::tuple_size_v<WidgetProperties>;
        inline static constexpr size_t index = []() constexpr {
            if constexpr (is_widget) {
                return _get_index<WidgetProperties>(name);
            } else {
                return _get_index<NodeProperties>(name);
            }}();
        using type = std::conditional_t<is_widget, widget_property_t<index>, node_property_t<index>>;
    };

    /// Helper struct to identify a Slot index, type and whether it is a Widget- or a Node Slot.
    template<char... Cs>
    struct _SlotStudy {
        inline static constexpr StringType name = StringType<Cs...>{};
        inline static constexpr bool is_widget = _get_index<WidgetSlots>(name) < std::tuple_size_v<NodeSlots>;
        inline static constexpr size_t index = []() constexpr {
            if constexpr (is_widget) {
                return _get_index<WidgetSlots>(name);
            } else {
                return _get_index<NodeSlots>(name);
            }}();
        using type = std::conditional_t<is_widget, widget_slot_t<index>, node_slot_t<index>>;
    };

    /// Helper struct to identify a Signal index, type and whether it is a Widget- or a Node Signal.
    template<char... Cs>
    struct _SignalStudy {
        inline static constexpr StringType name = StringType<Cs...>{};
        inline static constexpr bool is_widget = _get_index<WidgetSignals>(name) < std::tuple_size_v<NodeSignals>;
        inline static constexpr size_t index = []() constexpr {
                if constexpr (is_widget) {
                return _get_index<WidgetSignals>(name);
    } else {
                return _get_index<NodeSignals>(name);
    }}();
        using type = std::conditional_t<is_widget, widget_signal_t<index>, node_signal_t<index>>;
    };
    // clang-format on

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Value constructor.
    /// @param parent   Parent of this Node.
    Widget(valid_ptr<AnyNode*> parent)
        : AnyWidget(parent)
        , m_state(std::in_place_type_t<state_t<0>>(), *static_cast<typename state_t<0>::node_t*>(this)) {

        // properties
        for_each(m_widget_properties, [this](auto& property) {
            // create property
            using widget_property_t = typename std::decay_t<decltype(property)>::element_type;
            property = std::make_unique<widget_property_t>();

            // mark this Widget dirty whenever a visible property changes its value
            if (widget_property_t::policy_t::visibility != AnyProperty::Visibility::INVISIBILE) {
                property->get_operator()->subscribe(_get_redraw_observer());
            }

            // refresh the WidgetDesign whenever a REFRESH property changes its value
            if (widget_property_t::policy_t::visibility == AnyProperty::Visibility::REFRESH) {
                property->get_operator()->subscribe(_get_refresh_observer());
            }
        });

        // slots
        for_each(m_widget_slots, [this](auto& slot) {
            slot = std::make_unique<typename std::decay_t<decltype(slot)>::element_type>();
        });

        // signals
        for_each(m_widget_signals, [this](auto& signal) {
            signal = std::make_shared<typename std::decay_t<decltype(signal)>::element_type>();
        });
    }

    // state machine ----------------------------------------------------------
public:
    /// The name of the current State.
    std::string_view get_state_name() const noexcept final {
        return visit_at<std::string_view>(m_state, m_state.index(), [](const auto& state) -> std::string_view {
            return std::decay_t<decltype(state)>::name.get_view();
        });
    }

    /// @{
    /// Checks if a transition from one to the other State is possible.
    template<class CurrentState, class NewState>
    static constexpr bool is_valid_transition() noexcept {
        return std::is_constructible_v<NewState, std::add_rvalue_reference_t<CurrentState>>;
    }
    bool is_valid_transition(const std::string& from, std::string& to) const final {
        return _is_valid_transition(_get_index<StateTuple>(from), _get_index<StateTuple>(to));
    }
    /// @}

    /// @{
    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, class = std::enable_if_t<is_one_of_variant<NewState, StateVariant>()>>
    void transition_into() {
        _transition_into<NewState>();
    }

    template<char... Cs, class NewState = state_t<_get_index<StateTuple>(StringType<Cs...>{})>>
    void transition_into(StringType<Cs...>) {
        _transition_into<NewState>();
    }

    template<const ConstString& name>
    void transition_into() {
        return transition_into(make_string_type<name>());
    }

    void transition_into(const std::string& state) final {
        const size_t target_index = _get_index<StateTuple>(state);
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

    // properties -------------------------------------------------------------
public:
    /// @{
    /// Returns the correctly typed value of a Property.
    /// @param name     Name of the requested Property.
    template<char... Cs, class Study = _PropertyStudy<Cs...>>
    constexpr const auto& get(StringType<Cs...> name) const {
        if constexpr (Study::is_widget) {
            return std::get<Study::index>(m_widget_properties)->get();
        } else {
            return AnyWidget::get(name);
        }
    }
    template<const ConstString& name>
    constexpr const auto& get() const {
        return get(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Updates the value of a Property of this Node.
    /// @param name     Node-unique name of the Property.
    /// @param value    New value of the Property.
    template<class T, char... Cs, class Study = _PropertyStudy<Cs...>,
             class = std::enable_if_t<std::is_convertible_v<T, typename Study::type>>>
    constexpr void set(StringType<Cs...> name, T&& value) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_properties)->set(std::forward<T>(value));
        } else {
            AnyWidget::set(name, std::forward<T>(value));
        }
    }
    template<const ConstString& name, class T>
    constexpr void set(T&& value) {
        set(make_string_type<name>(), std::forward<T>(value));
    }
    /// @}

    /// @{
    /// Returns the correctly typed value of a Property.
    /// @param name     Name of the requested Property.
    template<char... Cs, class Study = _PropertyStudy<Cs...>>
    constexpr auto connect_property(StringType<Cs...> name) const {
        if constexpr (Study::is_widget) {
            return PropertyHandle(std::get<Study::index>(m_widget_properties).get());
        } else {
            return AnyWidget::connect_property(name);
        }
    }
    template<const ConstString& name>
    constexpr auto connect_property() const {
        return connect_property(make_string_type<name>());
    }
    /// @}

    // Use the node runtime methods alongside the widget's
    using AnyNode::connect_property;
    using AnyNode::get;
    using AnyNode::set;

    // signals / slots --------------------------------------------------------
public:
    /// @{
    /// Manually call the requested Slot of this Node.
    /// If T is not`None`, this method takes a second argument that is passed to the Slot.
    /// The Publisher of the Slot's `on_next` call id is set to `nullptr`.
    template<char... Cs, class Study = _SlotStudy<Cs...>>
    std::enable_if_t<std::is_same_v<typename Study::type, None>> call(StringType<Cs...> name) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_slots)->call();
        } else {
            return AnyWidget::call(name);
        }
    }

    template<char... Cs, class Study = _SlotStudy<Cs...>, class T = typename Study::type::value_t>
    std::enable_if_t<!std::is_same_v<T, None>> call(StringType<Cs...> name, const T& value) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_slots)->call(value);
        } else {
            return AnyWidget::call(name, value);
        }
    }

    template<const ConstString& name, class Study = _SlotStudy<decltype(make_string_type<name>())::Chars>,
             class T = typename Study::type::value_t>
    std::enable_if_t<std::is_same_v<T, None>> call() {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_slots)->call();
        } else {
            return AnyWidget::call<name>();
        }
    }

    template<const ConstString& name, class Study = _SlotStudy<decltype(make_string_type<name>())::Chars>,
             class T = typename Study::type::value_t>
    std::enable_if_t<!std::is_same_v<T, None>> call(const T& value) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_slots)->call(value);
        } else {
            return AnyWidget::call<name>(value);
        }
    }
    /// @}

    /// @{
    /// Returns the requested Slot or void (which doesn't compile).
    /// @param name     Name of the requested Slot.
    template<char... Cs, class Study = _SlotStudy<Cs...>>
    constexpr auto connect_slot(StringType<Cs...> name) const {
        if constexpr (Study::is_widget) {
            return SlotHandle(std::get<Study::index>(m_widget_slots).get());
        } else {
            return AnyWidget::connect_slot(name);
        }
    }
    template<const ConstString& name>
    constexpr auto connect_slot() const {
        return connect_slot(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Returns the requested Signal or void (which doesn't compile).
    /// @param name     Name of the requested Signal.
    template<char... Cs, class Study = _SignalStudy<Cs...>>
    constexpr auto connect_signal(StringType<Cs...> name) const {
        if constexpr (Study::is_widget) {
            return std::get<Study::index>(m_widget_signals);
        } else {
            return AnyWidget::connect_signal(name);
        }
    }
    template<const ConstString& name>
    constexpr auto connect_signal() const {
        return connect_signal(make_string_type<name>());
    }
    /// @}

    // Use the node runtime methods alongside the widget's
    using AnyNode::connect_signal;
    using AnyNode::connect_slot;

    // state machine ----------------------------------------------------------
protected:
    /// Current State of this Widget.
    const StateVariant& _get_current_state() const { return m_state; }

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

    /// Transitions from the current into the given State.
    /// @throws AnyWidget::BadTransitionError   If the transition is not possible.
    template<class NewState, std::size_t I = get_first_variant_index<NewState, StateVariant>()>
    void _transition_into() {
        if constexpr (is_valid_transition<state_t<I>, NewState>()) {
            m_state.template emplace<NewState>(std::move(std::get<I>(m_state)));
        } else {
            NOTF_THROW(AnyWidget::BadTransitionError,
                       "Cannot transition Node {} from State \"{}\" into State \"{}\'", //
                       this->get_uuid().to_string(), get_state_name(), NewState::name.c_str());
        }
    }

    /// Transitions frm the current into another State identified by index.
    template<size_t I = 0>
    void _transition_into(const size_t index) {
        if constexpr (I < std::variant_size_v<StateVariant>) {
            if (I == index) {
                return _transition_into<state_t<I>>();
            } else {
                return _transition_into<I + 1>(index);
            }
        } else {
            NOTF_ASSERT(false);
        }
    }

    // properties -------------------------------------------------------------
protected:
    /// @{
    /// (Re-)Defines a callback to be invoked every time the value of the Property is about to change.
    template<char... Cs, class Study = _PropertyStudy<Cs...>, class Callback = typename Study::type::callback_t>
    constexpr void _set_property_callback(StringType<Cs...> name, Callback callback) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_properties)->set_callback(std::forward<Callback>(callback));
        } else {
            AnyWidget::_set_property_callback(name, std::move(callback));
        }
    }
    template<const ConstString& name, class Study = _PropertyStudy<decltype(make_string_type<name>())::Chars>,
             class Callback = typename Study::type::callback_t>
    constexpr void _set_property_callback(Callback callback) {
        _set_property_callback(make_string_type<name>(), std::move(callback));
    }
    /// @}

    // Use the node runtime methods alongside the widget's
    using AnyNode::_set_property_callback;

    // signals / slots --------------------------------------------------------
protected:
    /// @{
    /// Internal access to a Slot of this Node.
    /// @param name     Name of the requested Property.
    template<char... Cs, class Study = _SlotStudy<Cs...>>
    constexpr auto _get_slot(StringType<Cs...> name) const {
        if constexpr (Study::is_widget) {
            return std::get<Study::index>(m_widget_slots)->get_publisher();
        } else {
            return AnyWidget::_get_slot(name);
        }
    }
    template<const ConstString& name>
    constexpr auto _get_slot() const {
        return _get_slot(make_string_type<name>());
    }
    /// @}

    /// @{
    /// Emits a Signal with a given value.
    /// @param name     Name of the Signal to emit.
    /// @param value    Data to emit.
    /// @throws         NameError / TypeError
    template<char... Cs, class Study = _SignalStudy<Cs...>, class T = typename Study::type::value_t,
             class = std::enable_if_t<!std::is_same_v<T, None>>>
    void _emit(StringType<Cs...> name, const T& value) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_signals)->publish(value);
        } else {
            AnyWidget::_emit(name, value);
        }
    }
    template<char... Cs, class Study = _SignalStudy<Cs...>, class T = typename Study::type::value_t,
             class = std::enable_if_t<std::is_same_v<T, None>>>
    void _emit(StringType<Cs...> name) {
        if constexpr (Study::is_widget) {
            std::get<Study::index>(m_widget_signals)->publish();
        } else {
            AnyWidget::_emit(name);
        }
    }
    /// @}

    // Use the node runtime methods alongside the widget's
    using AnyNode::_emit;
    using AnyNode::_get_slot;

private:
    /// Implementation specific query of a Property.
    AnyProperty* _get_property_impl(const std::string& name) const override {
        if (const size_t index = _get_index<WidgetProperties>(hash_string(name));
            index < std::tuple_size_v<WidgetProperties>) {
            return visit_at<AnyProperty*>(m_widget_properties, index,
                                          [](const auto& property) { return property.get(); });
        } else {
            return AnyWidget::_get_property_impl(name);
        }
    }

    /// Implementation specific query of a Slot, returns an empty pointer if no Slot by the given name is found.
    /// @param name     Node-unique name of the Slot.
    AnySlot* _get_slot_impl(const std::string& name) const override {
        if (const size_t index = _get_index<WidgetSlots>(hash_string(name)); index < std::tuple_size_v<WidgetSlots>) {
            return visit_at<AnySlot*>(m_widget_slots, index, [](const auto& slot) { return slot.get(); });
        } else {
            return AnyWidget::_get_slot_impl(name);
        }
    }

    /// Implementation specific query of a Signal, returns an empty pointer if no Signal by the given name is found.
    /// @param name     Node-unique name of the Signal.
    AnySignalPtr _get_signal_impl(const std::string& name) const override {
        if (const size_t index = _get_index<WidgetSignals>(hash_string(name));
            index < std::tuple_size_v<WidgetSignals>) {
            return visit_at<AnySignalPtr>(m_widget_signals, index, [](const auto& signal) { return signal; });
        } else {
            return AnyWidget::_get_signal_impl(name);
        }
    }

    /// Calculates the combined hash value of all Properties.
    size_t _calculate_property_hash(size_t result = detail::versioned_base_hash()) const override {
        for_each(
            m_widget_properties, [](auto& property, size_t& out) { hash_combine(out, property->get()); }, result);
        return AnyWidget::_calculate_property_hash(result);
    }

    /// Removes all modified data from all Properties.
    void _clear_modified_properties() override {
        for_each(m_widget_properties, [](auto& property) { property->clear_modified_data(); });
        AnyWidget::_clear_modified_properties();
    }

    // hide some protected methods
    using AnyNode::_get_redraw_observer;
    using AnyNode::_is_finalized;

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Current State of this Widget.
    StateVariant m_state;

    /// All Properties of this Node, default initialized to the Definition's default values.
    WidgetProperties m_widget_properties;

    /// All Slots of this Node.
    WidgetSlots m_widget_slots;

    /// All Signals of this Node.
    WidgetSignals m_widget_signals;
};

NOTF_CLOSE_NAMESPACE
