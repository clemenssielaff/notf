#pragma once

#include "notf/meta/concept.hpp"

#include "notf/common/delegate.hpp"

#include "notf/reactive/pipeline.hpp"

#include "notf/app/graph/graph.hpp"

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

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type of the (optional) callback that is invoked every time the value of the PropertyOperator is about to change.
    /// If the callback returns false, the update is cancelled and the old value remains.
    /// If the callback returns true, the update will proceed.
    /// Since the value is passed in by mutable reference, it can modify the value however it wants to. Even if the new
    /// value ends up the same as the old, the update will proceed. Note though, that the callback will only be called
    /// if the value is initially different from the one stored in the PropertyOperator.
    using callback_t = Delegate<bool(T&)>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(PropertyOperator);

    /// Value constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    PropertyOperator(T value, bool is_visible) : m_hash(is_visible ? hash(value) : 0), m_value(std::move(value)) {}

    /// Destructor.
    ~PropertyOperator() override { this->complete(); }

    /// Update the Property from upstream.
    void on_next(const AnyPublisher* /*publisher*/, const T& value) final {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        set(value);
    }

    void on_error(const AnyPublisher* /*publisher*/, const std::exception& exception) final {
        report_property_operator_error(exception);
    }

    /// Properties cannot be completed from the outside
    void on_complete(const AnyPublisher* /*publisher*/) final {}

    /// Latest value hash, or 0 if the Property is invisible.
    size_t get_hash() const { return m_hash; }

    /// Current value of the Property.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const T& get() const {
        if (!this_thread::is_the_ui_thread()) {
            return m_value; // the renderer always sees the unmodified value
        }

        // if there exist a modified value, return that one instead
        if (T* modified_value = m_modified_value.get()) { return *modified_value; }
        return m_value;
    }

    /// The Property value.
    /// @param value    New value.
    void set(const T& value) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());

        // do nothing if the property value would not actually change
        if (value == m_value) { return; }

        // give the optional callback the chance to modify/veto the change
        T new_value = value;
        if (m_callback && !m_callback(new_value)) { return; }

        // if this is the first modification, create a modified copy
        if (m_modified_value == nullptr) {
            m_modified_value = std::make_unique<T>(std::move(new_value));
        } else { // if a modified value already exists, update it
            *m_modified_value.get() = std::move(new_value);
        }

        // hash and publish the value
        if (m_hash != 0) { m_hash = hash(*m_modified_value.get()); }
        this->publish(*m_modified_value.get());
    }

    /// Installs a (new) callback that is invoked every time the value of the PropertyOperator is about to change.
    void set_callback(callback_t callback) {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        m_callback = std::move(callback);
    }

    /// Deletes the modified value copy, if one exists.
    void clear_modified_value() {
        NOTF_ASSERT(this_thread::is_the_ui_thread());
        if (m_modified_value) {
            m_value = std::move(*m_modified_value.get());
            m_modified_value.reset();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Pointer to a frozen copy of the value, if it was modified while the Graph was frozen.
    std::unique_ptr<T> m_modified_value;

    /// Property callback, executed before the value of the ProperyOperator would change.
    callback_t m_callback;

    /// Hash of the stored value.
    size_t m_hash;

    /// The stored value.
    T m_value;
};

} // namespace detail

// property ========================================================================================================= //

/// Base class for all Property types.
class AnyProperty {

    // types-- --------------------------------------------------------------------------------- //
public:
    /// Changing this Property ...
    enum class Visibility {
        INVISIBILE, ///> ... does not require any action from the renderer.
        REDRAW,     ///> ... requires the Node to be drawn again as it is (only transformed etc.).
        REFRESH,    ///> ... requires the Node to update and then redraw.
    };

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

// typed property =================================================================================================== //

/// A Typed Property.
/// Is not a concreate class yet, but unifies all value-related methods.
template<class T>
class TypedProperty : public AnyProperty {

    static_assert(std::is_move_constructible_v<T>, "Property values must be movable to freeze them");
    static_assert(is_hashable_v<T>, "Property values must be hashable");
    static_assert(!std::is_const_v<T>, "Property values must be modifyable");
    static_assert(true, "Property values must be serializeable"); // TODO: check property value types f/ serializability

    // types ----------------------------------------------------------------------------------- //
public:
    /// Property value type.
    using value_t = T;

    /// Type of Operator in this Property.
    using operator_t = std::shared_ptr<detail::PropertyOperator<T>>;

    /// Type of the (optional) callback that is invoked every time the value of the Property is about to change.
    using callback_t = typename detail::PropertyOperator<T>::callback_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Value constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    TypedProperty(T value, bool is_visible)
        : m_operator(std::make_shared<detail::PropertyOperator<T>>(std::move(value), is_visible)) {}

    /// Name of this Property type, for runtime reporting.
    std::string_view get_type_name() const final {
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
    const T& get() const noexcept { return m_operator->get(); }

    /// Updater the Property value and bind it.
    /// @param value    New Property value.
    void set(const T& value) { m_operator->set(value); }

    /// Reactive Property operator underlying the Property's reactive functionality.
    operator_t& get_operator() { return m_operator; }

    /// Installs a (new) callback that is invoked every time the value of the Property is about to change.
    void set_callback(callback_t callback) { m_operator->set_callback(std::move(callback)); }

    /// Deletes all modified data of this Property.
    void clear_modified_data() override { m_operator->clear_modified_value(); }

    // fields ---------------------------------------------------------------------------------- ///
private:
    /// Reactive Property operator, most of the Property's implementation.
    operator_t m_operator;
};

// property ========================================================================================================= //

namespace detail {

/// Validates a Property policy and completes partial policies.
template<class Policy>
class PropertyPolicyFactory {

    NOTF_CREATE_TYPE_DETECTOR(value_t);
    static_assert(has_value_t_v<Policy>, "A PropertyPolicy must contain the type of Property as type `value_t`");

    NOTF_CREATE_FIELD_DETECTOR(name);
    static_assert(has_name_v<Policy>,
                  "A PropertyPolicy must contain the name of the Property as `static constexpr name`");
    static_assert(std::is_same_v<decltype(Policy::name), const ConstString>,
                  "The name of a PropertyPolicy must be of type `ConstString`");

    NOTF_CREATE_FIELD_DETECTOR(default_value);

    NOTF_CREATE_FIELD_DETECTOR(visibility);
    static constexpr AnyProperty::Visibility create_visibility() {
        if constexpr (has_visibility_v<Policy>) {
            static_assert(std::is_same_v<decltype(Policy::visibility), const AnyProperty::Visibility>,
                          "The visibility of a PropertyPolicy must be of type `AnyProperty::Visibility`");
            return Policy::visibility;
        } else {
            return AnyProperty::Visibility::REDRAW; // by default
        }
    }

public:
    /// Validated and completed Property policy.
    struct PropertyPolicy {

        /// Mandatory value type of the Property Policy.
        using value_t = typename Policy::value_t;

        /// Mandatory name of the Property Policy.
        static constexpr ConstString name = Policy::name;

        /// Default value, either explicitly given by the user Policy or defaulted.
        /// Is a method, because not all types can be stored as a constexpr value.
        static value_t get_default_value() {
            if constexpr (has_default_value_v<Policy>) {
                static_assert(std::is_convertible_v<decltype(Policy::default_value), typename Policy::value_t>,
                              "The default value of a PropertyPolicy must be convertible to its type");
                return Policy::default_value; // explicit default value
            } else if constexpr (std::is_arithmetic_v<Policy::value_t>) {
                return 0; // zero for numeric types
            } else {
                return Policy::value_t(); // default initialized value
            }
        }

        /// Whether the Property is visible, either explicitly given by the user Policy or REDRAW by default.
        static constexpr AnyProperty::Visibility visibility = create_visibility();
    };
};

} // namespace detail

/// Example Policy:
///
///     struct PropertyPolicy {
///         using value_t = float;
///         static constexpr ConstString name = "position";
///         static constexpr value_t default_value = 0.123f;
///         static constexpr AnyProperty::Visibility visibility = AnyProperty::Visibility::REDRAW;
///     };
///
template<class Policy>
class Property final : public TypedProperty<typename Policy::value_t> {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Property type.
    using policy_t = typename detail::PropertyPolicyFactory<Policy>::PropertyPolicy;

    /// Property value type.
    using value_t = typename policy_t::value_t;

    /// The compile time constant name of this Property.
    static constexpr const ConstString& name = policy_t::name;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    Property(value_t value = policy_t::get_default_value(), AnyProperty::Visibility visibility = policy_t::visibility)
        : TypedProperty<value_t>(std::move(value), (visibility != AnyProperty::Visibility::INVISIBILE)) {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const final { return name.c_str(); }

    /// The default value of this Property.
    const value_t& get_default() const final {
        static const value_t default_value = policy_t::get_default_value();
        return default_value;
    }
};

// property handle ================================================================================================== //

/// Object wrapping a weak_ptr to a Property. Is returned by Node::connect_property and can safely be stored &
/// copied anywhere.
template<class T>
class PropertyHandle {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Operator type of the handled Property type.
    using operator_t = typename TypedProperty<T>::operator_t;

    /// Weak pointer to the Property's Operator type.
    using weak_operator_t = typename operator_t::weak_type;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param property Property to Handle.
    PropertyHandle(valid_ptr<TypedProperty<T>*> property) : m_operator(property->get_operator()) {}

    /// Reactive Pipeline "|" operator
    /// Connects the Property on the left.
    template<class Sub, class DecayedSub = std::decay_t<Sub>>
    friend std::enable_if_t<detail::is_reactive_compatible_v<operator_t, DecayedSub>, Pipeline<DecayedSub>>
    operator|(const PropertyHandle& property, Sub&& subscriber) {
        operator_t operator_ptr = property.m_operator.lock();
        if (!operator_ptr) { NOTF_THROW(HandleExpiredError, "PropertyHandle is expired"); }
        return operator_ptr | std::forward<Sub>(subscriber);
    }

    /// Reactive Pipeline "|" operator
    /// Connect the Property on the right.
    template<class Pub>
    friend std::enable_if_t<detail::is_reactive_compatible_v<std::decay_t<Pub>, operator_t>, Pipeline<operator_t>>
    operator|(Pub&& publisher, const PropertyHandle& property) {
        operator_t operator_ptr = property.m_operator.lock();
        if (!operator_ptr) { NOTF_THROW(HandleExpiredError, "PropertyHandle is expired"); }
        return std::forward<Pub>(publisher) | operator_ptr;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Operator of the handled Property.
    weak_operator_t m_operator;
};

NOTF_CLOSE_NAMESPACE
