#pragma once

#include "notf/meta/stringtype.hpp"
#include "notf/reactive/pipeline.hpp"

#include "./graph.hpp"

NOTF_OPEN_NAMESPACE

// property operator ================================================================================================ //

namespace detail {

/// The Reactive Property Operator contains most of the Property-related functionality like caching and hashing.
/// The actual `Property` class acts as more of a facade.
template<class T>
class PropertyOperator : public Operator<T, T, detail::MultiPublisherPolicy> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(PropertyOperator);

    /// Value constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    PropertyOperator(T value, bool is_visible) : m_hash(is_visible ? hash(m_value) : 0), m_value(std::move(value)) {}

    /// Destructor.
    ~PropertyOperator() override
    {
        clear_frozen();
        this->complete();
    }

    void on_next(const AnyPublisher* /*publisher*/, const T& value) final { set(value); }

    void on_error(const AnyPublisher* /*publisher*/, const std::exception& /*exception*/) final
    {
        // TODO: how does a Property handle upstream errors? It should swallow it and not pass it on downstream, but
        // do we just report it? re-throw?
    }

    /// Properties cannot be completed from the outside
    void on_complete(const AnyPublisher* /*publisher*/) final {}

    /// Latest value hash, or 0 if the Property is invisible.
    size_t get_hash() const { return m_hash; }

    /// Current value of the Property.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const T& get(const std::thread::id thread_id) const
    {
        if (T* frozen_value = m_cache.load(std::memory_order_consume)) {
            if (TheGraph::is_frozen_by(thread_id)) { return *frozen_value; }
        }
        return m_value;
    }

    /// The Property value.
    /// @param value    New value.
    void set(const T& value)
    {
        // do nothing if the property value would not actually change
        if (value == m_value) { return; }

        // if the graph is currently frozen and this is the first modification of this property,
        // create a frozen copy of the current value before changing it
        if (T* frozen_value = m_cache.load(std::memory_order_relaxed); frozen_value == nullptr) {
            if (TheGraph::is_frozen()) { m_cache.store(new T(std::move(m_value)), std::memory_order_release); }
        }

        // update and publish
        m_value = value;
        if (m_hash != 0) { m_hash = hash(m_value); }
        this->publish(m_value);
    }

    /// Deletes the frozen value copy, if one exists.
    void clear_frozen() { delete m_cache.exchange(nullptr); }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// Pointer to a frozen copy of the value, if it was modified while the Graph was frozen.
    std::atomic<T*> m_cache{nullptr};

    /// Hash of the stored value.
    size_t m_hash;

    /// The stored value.
    T m_value;
};

} // namespace detail

// property ========================================================================================================= //

/// Base class for all Property types.
class AnyProperty {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    NOTF_NO_COPY_OR_ASSIGN(AnyProperty);

    /// Default Constructor.
    AnyProperty() = default;

    /// Destructor.
    virtual ~AnyProperty() = default;

    /// Name of this Property type, for runtime reporting.
    virtual std::string_view get_type_name() const = 0;

    /// The hash of this Property's value.
    virtual size_t get_hash() const = 0;
};

template<class T>
class Property : public AnyProperty {

    static_assert(std::is_move_constructible_v<T>, "Property values must be movable to freeze them");
    static_assert(is_hashable_v<T>, "Property values must be hashable");
    static_assert(!std::is_const_v<T>, "Property values must be modifyable");
    static_assert(!std::is_same_v<T, None>, "Property value type must not be None");
    static_assert(true, "Property values must be serializeable"); // TODO: check property value types f/ serializability

    template<class Pub, class Prop, class, class>
    friend std::enable_if_t<
        detail::is_reactive_compatible_v<
            std::decay_t<Pub>, std::shared_ptr<detail::PropertyOperator<typename Prop::element_type::value_t>>>,
        Pipeline<std::shared_ptr<detail::PropertyOperator<typename Prop::element_type::value_t>>>>
    operator|(Pub&& publisher, Prop& property);

    friend struct Accessor<Property<T>, Node>;

    // types -------------------------------------------------------------------------------------------------------- //
public:
    template<class Other>
    using AccessFor = Accessor<Property<T>, Other>;

    /// Property value type.
    using value_t = T;

    /// Type of Operator in this Property.
    using operator_t = std::shared_ptr<detail::PropertyOperator<T>>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Value constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    Property(T value, bool is_visible)
        : m_operator(std::make_shared<detail::PropertyOperator<T>>(std::move(value), is_visible))
    {}

    /// Name of this Property type, for runtime reporting.
    std::string_view get_type_name() const final
    {
        static const std::string my_type = type_name<T>();
        return my_type;
    }

    /// The Node-unique name of this Property.
    virtual std::string_view get_name() const = 0;

    /// The hash of this Property's value.
    size_t get_hash() const override { return m_operator->get_hash(); }

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool is_visible() const noexcept { return get_hash() != 0; }

    /// The Property value.
    const T& get() const noexcept { return m_operator->get(std::this_thread::get_id()); }

    /// Updater the Property value and bind it.
    /// @param value    New Property value.
    void set(const T& value) { m_operator->set(value); }

    /// Connect the reactive Operator of this Property to a Subscriber downstream.
    template<class Prop, class Sub>
    friend std::enable_if_t<
        detail::is_reactive_compatible_v<operator_t, std::decay_t<Sub>> && std::is_base_of_v<Property<T>, Prop>,
        Pipeline<std::decay_t<Sub>>>
    operator|(std::shared_ptr<Prop>& property, Sub&& subscriber)
    {
        return property->m_operator | std::forward<Sub>(subscriber);
    }

    /// Connect a Publisher upstream to a Property.
    template<class Pub, class Prop>
    friend std::enable_if_t<
        detail::is_reactive_compatible_v<std::decay_t<Pub>, operator_t> && std::is_base_of_v<Property<T>, Prop>,
        Pipeline<operator_t>>
    operator|(Pub&& publisher, std::shared_ptr<Prop>& property)
    {
        return std::forward<Pub>(publisher) | property->m_operator;
    }

    /// Connect a Property upstream to a Property downstream.
    template<class Pub, class Sub>
    friend std::enable_if_t<
        detail::is_reactive_compatible_v<operator_t, typename Sub::operator_t> && std::is_base_of_v<Property<T>, Pub>,
        Pipeline<typename Sub::operator_t>>
    operator|(std::shared_ptr<Pub>& lhs, std::shared_ptr<Sub>& rhs)
    {
        return lhs->m_operator | rhs->m_operator;
    }

    auto& get_operator() { return m_operator; }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// Reactive Property operator, most of the Property's implementation.
    operator_t m_operator;
};

// node accessor ==================================================================================================== //

/// Access to selected members of TheGraph.
template<class T>
class Accessor<Property<T>, Node> {
    friend Node;

    template<class>
    friend class CompileTimeNode;

    /// Reactive Property operator underlying the Property's reactive functionality.
    static auto& get_operator(PropertyPtr<T>& property) { return property->m_operator; }
};

// run time property ================================================================================================ //

template<class T>
class RunTimeProperty final : public Property<T> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param name         The Node-unique name of this Property.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    RunTimeProperty(std::string_view name, T value, bool is_visible = true)
        : Property<T>(std::move(value), is_visible), m_name(std::move(name))
    {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const final { return m_name; }

    // members ------------------------------------------------------------------------------------------------------ //
private:
    /// The Node-unique name of this Property.
    const std::string_view m_name;
};

// compile time property ============================================================================================ //

/// Example Policy:
///
///     struct PropertyPolicy {
///         using value_t = float;
///         static constexpr StringConst name = "position";
///         static constexpr value_t default_value = 0.123f;
///         static constexpr bool is_visible = true;
///     };
///
template<class Policy>
class CompileTimeProperty final : public Property<typename Policy::value_t> {

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    CompileTimeProperty(typename Policy::value_t value = Policy::default_value, bool is_visible = Policy::is_visible)
        : Property<typename Policy::value_t>(value, is_visible)
    {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const final { return Policy::name.c_str(); }

    /// The compile time constant name of this Property.
    static constexpr const StringConst& get_const_name() noexcept { return Policy::name; }
};

NOTF_CLOSE_NAMESPACE
