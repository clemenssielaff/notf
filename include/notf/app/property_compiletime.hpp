#pragma once

#include "notf/meta/stringtype.hpp"

#include "notf/app/property.hpp"

NOTF_OPEN_NAMESPACE

// property policy factory ========================================================================================== //

namespace detail {

/// Validates a Property policy and completes partial policies.
struct PropertyPolicyFactory {

    /// Factory method.
    template<class Policy>
    static constexpr auto create() {
        // validate the given Policy and show an appropriate error message if something goes wrong
        static_assert(decltype(has_value_t<Policy>(std::declval<Policy>()))::value,
                      "A PropertyPolicy must contain the type of Property as type `value_t`");

        static_assert(decltype(has_name<Policy>(std::declval<Policy>()))::value,
                      "A PropertyPolicy must contain the name of the Property as `static constexpr name`");
        static_assert(std::is_same_v<decltype(Policy::name), const StringConst>,
                      "The name of a PropertyPolicy must be of type `StringConst`");

        if constexpr (decltype(has_default_value<Policy>(std::declval<Policy>()))::value) {
            static_assert(std::is_convertible_v<decltype(Policy::default_value), typename Policy::value_t>,
                          "The default value of a PropertyPolicy must be convertible to its type");
        }

        if constexpr (decltype(has_is_visible<Policy>(std::declval<Policy>()))::value) {
            static_assert(std::is_same_v<decltype(Policy::is_visible), const bool>,
                          "The visibility flag a PropertyPolicy must be a boolean");
        }

        /// Validated and completed Property policy.
        struct PropertyPolicy {
            /// Mandatory value type of the Property Policy.
            using value_t = typename Policy::value_t;

            /// Mandatory name of the Property Policy.
            static constexpr const StringConst& get_name() { return Policy::name; }

            /// Default value, either explicitly given by the user Policy or defaulted.
            static constexpr value_t get_default() {
                if constexpr (decltype(has_default_value<Policy>(std::declval<Policy>()))::value) {
                    return Policy::default_value; // explicit default value
                } else if constexpr (std::is_arithmetic_v<value_t>) {
                    return 0; // zero for numeric types
                } else {
                    return {}; // default initialized value
                }
            }

            /// Whether the Property is visible, either explicitly given by the user Policy or true by default.
            static constexpr bool is_visible() {
                if constexpr (decltype(has_is_visible<Policy>(std::declval<Policy>()))::value) {
                    return Policy::is_visible;
                } else {
                    return true; // visible by default
                }
            }
        };

        return PropertyPolicy();
    }

    /// Checks, whether the given type has a nested type `value_t`.
    template<class T>
    static constexpr auto has_value_t(const T&) -> decltype(std::declval<typename T::value_t>(), std::true_type{});
    template<class>
    static constexpr auto has_value_t(...) -> std::false_type;

    /// Checks, whether the given type has a static field `name`.
    template<class T>
    static constexpr auto has_name(const T&) -> decltype(T::name, std::true_type{});
    template<class>
    static constexpr auto has_name(...) -> std::false_type;

    /// Checks, whether the given type has a static field `default_value`.
    template<class T>
    static constexpr auto has_default_value(const T&) -> decltype(T::default_value, std::true_type{});
    template<class>
    static constexpr auto has_default_value(...) -> std::false_type;

    /// Checks, whether the given type has a static field `is_visible`.
    template<class T>
    static constexpr auto has_is_visible(const T&) -> decltype(T::is_visible, std::true_type{});
    template<class>
    static constexpr auto has_is_visible(...) -> std::false_type;
};

} // namespace detail

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

    // types ----------------------------------------------------------------------------------- //
public:
    /// Policy type as passed in by the user.
    using user_policy_t = Policy;

    /// Policy used to create this Property type.
    using policy_t = decltype(detail::PropertyPolicyFactory::create<Policy>());

    /// Property value type.
    using value_t = typename policy_t::value_t;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param value        Property value.
    /// @param is_visible   Whether a change in the Property will cause the Node to redraw or not.
    CompileTimeProperty(value_t value = policy_t::get_default(), bool is_visible = policy_t::is_visible())
        : Property<value_t>(std::move(value), is_visible) {}

    /// The Node-unique name of this Property.
    std::string_view get_name() const final { return get_const_name().c_str(); }

    /// The default value of this Property.
    const value_t& get_default() const final {
        static const value_t default_value = policy_t::get_default();
        return default_value;
    }

    /// The compile time constant name of this Property.
    static constexpr const StringConst& get_const_name() noexcept { return policy_t::get_name(); }
};

NOTF_CLOSE_NAMESPACE
