#include <iostream>

#include "notf/common/string_view.hpp"
#include "notf/common/variant.hpp"
#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/log.hpp"
#include "notf/meta/stringtype.hpp"
#include "notf/meta/time.hpp"

#pragma GCC diagnostic ignored "-Wweak-vtables"
#pragma GCC diagnostic ignored "-Wpadded"

NOTF_USING_NAMESPACE;

// property ========================================================================================================= //

struct UntypedProperty {
    virtual ~UntypedProperty() = default;
};

template<class T>
struct Property : public UntypedProperty {
    using type_t = T;

    /// Value constructor.
    Property(T value, bool is_visible) : m_value(std::forward<T>(value)), m_is_visible(is_visible) {}

    const T& get() const noexcept { return m_value; }

    void set(T value) { m_value = std::forward<T>(value); }

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool is_visible() noexcept { return m_is_visible; }

    // members ------------------------------------------------------------------------------------------------------ //
protected:
    type_t m_value;

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool m_is_visible;
};

template<class Trait>
struct CompileTimeProperty final : public Property<typename Trait::type_t> {

    /// Constructor.
    CompileTimeProperty() : Property<typename Trait::type_t>(Trait::default_value, Trait::is_visible) {}

    /// The name of this Property.
    static constexpr const StringConst& get_name() noexcept { return Trait::name; }

    /// Whether a change in the Property will cause the Node to redraw or not.
    static constexpr bool is_visible() noexcept { return Trait::is_visible; }

    /// Compile-time hash of the name of this Property.
    constexpr static size_t get_hash() noexcept { return Trait::name.get_hash(); }
};

// node ============================================================================================================= //

struct Node {

    /// Destructor.
    virtual ~Node() = default;

    /// Run time access to a Property of this Node.
    template<class T>
    const Property<T>& get_property(std::string_view name) const
    {
        const UntypedProperty* property = _get_property(std::move(name));
        if (const auto typed_property = dynamic_cast<const Property<T>*>(property)) {
            return *typed_property;
        }
        throw std::out_of_range("");
    }
    template<class T>
    Property<T>& get_property(std::string_view name)
    {
        return const_cast<Property<T>&>(const_cast<const Node*>(this)->get_property<T>(std::move(name)));
    }

protected:
    virtual const UntypedProperty* _get_property(std::string_view name) const = 0;
};

template<class Traits>
class CompileTimeNode : public Node {

    // types -------------------------------------------------------------------------------------------------------- //
protected:
    /// Tuple containing all compile time Properties of this Node type.
    using properties_t = typename Traits::properties;

    /// Type of a specific Property in this Node type.
    template<std::size_t I>
    using property_t = std::tuple_element_t<I, properties_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    static constexpr auto get_property_count() noexcept { return std::tuple_size_v<properties_t>; }

    /// We are using the run time `get_property` method in addition to the ones provided for compile time access.
    using Node::get_property;

    template<char... Cs>
    constexpr const auto& get_property(StringType<char, Cs...> name) const
    {
        return _get_property_by_name<0>(name);
    }
    template<char... Cs>
    constexpr auto& get_property(StringType<char, Cs...> name)
    {
        const auto& const_result = const_cast<const CompileTimeNode*>(this)->get_property(name);
        return const_cast<std::remove_const_t<decltype(const_result)>>(const_result);
    }

protected:
    const UntypedProperty* _get_property(std::string_view name) const override
    {
        return _get_property_by_hash<>(hash_string(name));
    }

private:
    template<size_t I, char... Cs>
    constexpr auto& _get_property_by_name(StringType<char, Cs...> name) const
    {
        if constexpr (property_t<I>::get_name() == name) {
            return std::get<I>(m_properties);
        }
        else if constexpr (I + 1 < get_property_count()) {
            return _get_property_by_name<I + 1>(name);
        }
        else {
            throw 0;
        }
    }

    template<std::size_t I = 0>
    const UntypedProperty* _get_property_by_hash(const size_t hash_value) const
    {
        constexpr size_t property_hash = property_t<I>::get_hash();
        if (property_hash == hash_value) {
            return &std::get<I>(m_properties);
        }
        if constexpr (I + 1 == get_property_count()) {
            return nullptr;
        }
        else {
            static_assert(I + 1 < get_property_count());
            return _get_property_by_hash<I + 1>(hash_value);
        }
    }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// All Properties of this Node, default initialized to the Definition's default values.
    properties_t m_properties;
};

// state ============================================================================================================ //

template<template<class> class ThisType, class NodeType>
struct State {
protected:
    /// State objects should not be instantiated
    State(NodeType& node) : m_node(node) {}

public:
    /// forbid the move constructor, otherwise all states would always be able to transition to themselves
    /// (can be overridden in subclasses)
    State(State&&) = delete;

    /// Virtual Destructor.
    virtual ~State() = default;

    void callback()
    {
        // default implementation (is ignored, should be non-virtually overwritten in StateType)
    }

protected:
    template<template<class> class T>
    constexpr void _transition_into()
    {
        static_assert(NodeType::template can_transition<ThisType<NodeType>, T<NodeType>>(), "Undefined State switch");
        m_node.template transition_into<T<NodeType>>();
    }

public:
    NodeType& m_node;
};

// widget =========================================================================================================== //

namespace detail {

struct PositionPropertyTrait {
    using type_t = float;
    static constexpr StringConst name = "position";
    static constexpr type_t default_value = 0.123f;
    static constexpr bool is_visible = true;
};
struct VisibilityPropertyTrait {
    using type_t = bool;
    static constexpr StringConst name = "visible";
    static constexpr type_t default_value = true;
    static constexpr bool is_visible = true;
};

struct WidgetTrait {
    using properties = std::tuple<                     //
        CompileTimeProperty<PositionPropertyTrait>,    //
        CompileTimeProperty<VisibilityPropertyTrait>>; //
};

} // namespace detail

/// Base class for all Widget types.
/// We know that all Widgets share a few common Propreties at compile time. The Widget class defines the compile time
/// Properties of all Widgets, as well as a virtual interface for all other Widget types at compile or run time.
class Widget : public CompileTimeNode<::detail::WidgetTrait> {

    // types -------------------------------------------------------------------------------------------------------- //
protected:
    /// Parent Node type.
    using node_t = CompileTimeNode<::detail::WidgetTrait>;

    /// Tuple containing all compile time Properties in the parent Node type.
    using node_properties_t = node_t::properties_t;

    /// Type of a specific Property in the parent Node type.
    template<std::size_t I>
    using node_property_t = node_t::property_t<I>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    virtual void paint() = 0;
};

template<class Traits>
class CompileTimeWidget : public Widget {

    /// Tuple containing all compile time Properties of this Widget type.
    using properties_t = typename Traits::properties;

    /// Type of a specific Property in this Widget type.
    template<size_t I>
    using widget_property_t = std::tuple_element_t<I, properties_t>;

    /// Variant of all States of this Widget type.
    using state_variant_t = typename Traits::states;

    /// Type of a specific State of thiw Widget.
    template<size_t I>
    using state_t = std::variant_alternative_t<I, state_variant_t>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    CompileTimeWidget() : Widget(), m_state(std::in_place_type_t<state_t<0>>(), *this) {}

    static constexpr auto get_property_count() noexcept
    {
        return _get_widget_property_count() + node_t::get_property_count();
    }

    /// We are using the runtime `get_property` method in addition to the ones provided for compile time access.
    using Node::get_property;

    template<char... Cs>
    constexpr const auto& get_property(StringType<char, Cs...> name) const
    {
        return _get_widget_property_by_name<0>(name);
    }
    template<char... Cs>
    constexpr auto& get_property(StringType<char, Cs...> name)
    {
        const auto& const_result = const_cast<const CompileTimeWidget*>(this)->get_property(name);
        return const_cast<std::remove_const_t<decltype(const_result)>>(const_result);
    }

    template<class CurrentState, class NewState>
    static constexpr bool can_transition()
    {
        return std::is_constructible_v<NewState, std::add_rvalue_reference_t<CurrentState>>;
    }

    template<class NewState, std::size_t I = 0>
    void transition_into()
    {
        if constexpr (I >= std::variant_size_v<state_variant_t>) {
            throw std::out_of_range("");
        }
        else {
            if constexpr (can_transition<state_t<I>, NewState>()) {
                if (m_state.index() == I) {
                    m_state.template emplace<NewState>(std::move(std::get<I>(m_state)));
                    return;
                }
            }
            return transition_into<NewState, I + 1>();
        }
    }

private:
    static constexpr auto _get_widget_property_count() noexcept { return std::tuple_size_v<properties_t>; }

    template<size_t I, char... Cs>
    constexpr auto& _get_widget_property_by_name(StringType<char, Cs...> name) const
    {
        if constexpr (widget_property_t<I>::get_name() == name) {
            return std::get<I>(m_widget_properties);
        }
        else if constexpr (I + 1 < _get_widget_property_count()) {
            return _get_widget_property_by_name<I + 1>(name);
        }
        else {
            return node_t::get_property(name);
        }
    }

    // members ------------------------------------------------------------------------------------------------------ //
protected:
    /// All Properties of this Widget, default initialized to the Trait's default values.
    properties_t m_widget_properties;

    /// Current State of this Widget.
    state_variant_t m_state;
};

// main ============================================================================================================= //

struct WeirdPropertyTrait {
    using type_t = int;
    static constexpr StringConst name = "soweird";
    static constexpr type_t default_value = -321;
    static constexpr bool is_visible = true;
};

template<class>
struct StateA;
template<class>
struct StateB;
template<class>
struct StateC;

#define TRANSITION_INTO(x)                \
    this->template _transition_into<x>(); \
    return;

template<class NodeType>
struct StateA : State<StateA, NodeType> {
    using node_t = NodeType;
    using parent_t = State<StateA, node_t>;

    // start State must have a "default" constructor
    explicit StateA(NodeType& node) : parent_t(node) { std::cout << "Default constructed A" << std::endl; }
    ~StateA() override { std::cout << "Destroyed A" << std::endl; }

    void callback() { TRANSITION_INTO(StateB); }
};

template<class NodeType>
struct StateB : State<StateB, NodeType> {
    using node_t = NodeType;
    using parent_t = State<StateB, node_t>;

    explicit StateB(StateA<NodeType>&& a) : parent_t(a.m_node) { std::cout << "Transitioned from A -> B" << std::endl; }
    ~StateB() override { std::cout << "Destroyed B" << std::endl; }

    void callback() { TRANSITION_INTO(StateC); }
};

template<class NodeType>
struct StateC : State<StateC, NodeType> {
    using node_t = NodeType;
    using parent_t = State<StateC, node_t>;

    explicit StateC(StateB<NodeType>&& b) : parent_t(b.m_node) { std::cout << "Transitioned from B -> C" << std::endl; }
    ~StateC() override { std::cout << "Destroyed C" << std::endl; }

    void callback() { std::cout << "Done :)" << std::endl; }
};

int main()
{
    NOTF_USING_LITERALS_NAMESPACE;

    struct Foo {};
    struct Bar {};

    using YESSA = tuple_to_variant_t<std::tuple<Foo, Bar>>;
    static_assert(std::is_same_v<YESSA, std::variant<Foo, Bar>>);

    struct TraitExample {
        using node_t = CompileTimeWidget<TraitExample>;
        using properties = std::tuple<CompileTimeProperty<WeirdPropertyTrait>>;
        using states = std::variant<StateA<node_t>, StateB<node_t>, StateC<node_t>>;
    };

    const CompileTimeNode<TraitExample> node;
    std::cout << node.get_property<int>("soweird").get() << std::endl;
    std::cout << node.get_property("soweird"_id).get() << std::endl;

    struct TestWidget : public TraitExample::node_t {
        void paint() override {}
        void run_callback()
        {
            std::visit(overloaded{[](auto&& state) { state.callback(); }}, m_state);
        }
    } widget;
    std::cout << widget.get_property<float>("position").get() << std::endl;
    std::cout << widget.get_property("visible"_id).get() << std::endl;
    std::cout << widget.get_property("soweird"_id).get() << std::endl;
    std::cout << widget.get_property("visible"_id).get() << std::endl;

    widget.run_callback();
    widget.run_callback();
    widget.run_callback();

    TheLogger::Args default_args;
    default_args.file_name = "log.txt";
    TheLogger::initialize(default_args);
    NOTF_LOG_TRACE("derbe aufs {} maul", "fiese");
    //    NOTF_THROW(value_error, "Junge, wa' {} fies", "dat");

    //    NOTF_ASSERT(0, "wasgehtn {} oderwas", "deinemudda");

    return 0;
}
