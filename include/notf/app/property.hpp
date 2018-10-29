#pragma once

#include "notf/reactive/pipeline.hpp"

#include "./graph.hpp"

NOTF_OPEN_NAMESPACE

// property operator ================================================================================================ //

namespace detail {

/// Reports (and ultimately ignores) an exception that was propagated to a PropertyOperator via `on_error`.
/// @param exception    Reported exception.
void report_property_operator_error(const std::exception& exception);

/// The Reactive Property Operator contains most of the Property-related functionality like caching and hashing.
/// The actual `Property` class acts as more of a facade.
template<class T>
class PropertyOperator : public Operator<T, T, detail::MultiPublisherPolicy> {

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(PropertyOperator);

    /// Value constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    PropertyOperator(T value, bool is_visible) : m_hash(is_visible ? hash(m_value) : 0), m_value(std::move(value)) {}

    /// Destructor.
    ~PropertyOperator() override
    {
        clear_modified_value();
        this->complete();
    }

    /// Update the Property from upstream.
    void on_next(const AnyPublisher* /*publisher*/, const T& value) final
    {
        NOTF_GUARD(std::lock_guard(TheGraph::get_graph_mutex()));
        set(value);
    }

    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) final
    {
        report_property_operator_error(exception);
    }

    /// Properties cannot be completed from the outside
    void on_complete(const AnyPublisher* /*publisher*/) final {}

    /// Latest value hash, or 0 if the Property is invisible.
    size_t get_hash() const { return m_hash; }

    /// Current value of the Property.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const T& get(const std::thread::id thread_id) const
    {
        if (TheGraph::is_frozen_by(thread_id)) {
            return m_value; // the renderer always sees the unmodified value
        }

        // if there exist a modified value, return that one instead
        if (T* modified_value = m_modified_value.get()) {
            NOTF_ASSERT(TheGraph::is_frozen());
            return *modified_value;
        }

        // either the value has not been modified, or the Graph is not frozen
        return m_value;
    }

    /// The Property value.
    /// @param value    New value.
    void set(const T& value)
    {
        NOTF_ASSERT(TheGraph::get_graph_mutex().is_locked_by_this_thread());

        // do nothing if the property value would not actually change
        if (value == m_value) { return; }

        // if the graph is currently frozen and this is the first modification of this property,
        // create a modified value copy
        if (TheGraph::is_frozen()) {
            // the render thread must never modify a Property
            NOTF_ASSERT(!TheGraph::is_frozen_by(std::this_thread::get_id()));

            // if this is the first modification, create a modified copy
            if (m_modified_value == nullptr) { m_modified_value = std::make_unique<T>(value); }

            // if a modified value already exists, update it
            else {
                *m_modified_value.get() = value;
            }

            // hash and publish the modified copy just like your regular value
            if (m_hash != 0) { m_hash = hash(*m_modified_value.get()); }
            this->publish(*m_modified_value.get());
        }

        // if the graph is not frozen, just update, hash and publish your regular value
        else {
            m_value = value;
            if (m_hash != 0) { m_hash = hash(m_value); }
            this->publish(m_value);
        }
    }

    /// Deletes the modified value copy, if one exists.
    void clear_modified_value()
    {
        if (m_modified_value) {
            m_value = std::move(*m_modified_value.get());
            m_modified_value.reset();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Pointer to a frozen copy of the value, if it was modified while the Graph was frozen.
    std::unique_ptr<T> m_modified_value;

    /// Hash of the stored value.
    size_t m_hash;

    /// The stored value.
    T m_value;
};

} // namespace detail

// property ========================================================================================================= //

/// Base class for all Property types.
class AnyProperty {

    // methods --------------------------------------------------------------------------------- //
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

    /// Deletes all modified data of this Property.
    virtual void clear_modified_data() = 0;
};

/// A Typed Property.
template<class T>
class Property : public AnyProperty {

    static_assert(std::is_move_constructible_v<T>, "Property values must be movable to freeze them");
    static_assert(is_hashable_v<T>, "Property values must be hashable");
    static_assert(!std::is_const_v<T>, "Property values must be modifyable");
    static_assert(true, "Property values must be serializeable"); // TODO: check property value types f/ serializability

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(Property<T>);

    /// Property value type.
    using value_t = T;

    /// Type of Operator in this Property.
    using operator_t = std::shared_ptr<detail::PropertyOperator<T>>;

    // methods --------------------------------------------------------------------------------- //
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

    /// The default value of this Property.
    virtual const T& get_default() const = 0;

    /// The Property value.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const T& get(const std::thread::id thread_id = std::this_thread::get_id()) const noexcept
    {
        return m_operator->get(thread_id);
    }

    /// Updater the Property value and bind it.
    /// @param value    New Property value.
    void set(const T& value) { m_operator->set(value); }

    /// Reactive Property operator underlying the Property's reactive functionality.
    operator_t& get_operator() { return m_operator; }

    /// Deletes all modified data of this Property.
    void clear_modified_data() override { m_operator->clear_modified_value(); }

    // fields ---------------------------------------------------------------------------------- ///
private:
    /// Reactive Property operator, most of the Property's implementation.
    operator_t m_operator;
};

// run time property ================================================================================================ //

template<class T>
class RunTimeProperty final : public Property<T> {

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param name         The Node-unique name of this Property.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    RunTimeProperty(std::string_view name, T value, bool is_visible = true)
        : Property<T>(value, is_visible), m_name(std::move(name)), m_default_value(std::move(value))
    {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const final { return m_name; }

    /// The default value of this Property.
    const T& get_default() const final { return m_default_value; }

    // fields ---------------------------------------------------------------------------------- ///
private:
    /// The Node-unique name of this Property.
    const std::string_view m_name;

    /// The default value of this Property.
    const T m_default_value;
};


NOTF_CLOSE_NAMESPACE
