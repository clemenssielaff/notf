#pragma once

#include <unordered_map>
#include <vector>

#include "notf/common/any.hpp"
#include "notf/meta/function.hpp"
#include "notf/meta/typename.hpp"

#include "./reactive_operator.hpp"

NOTF_OPEN_NAMESPACE

// reactive operator factory ======================================================================================== //

/// Base class for any reactive operator factory.
/// Has a templated `create` function that takes all of its arguments and packs them into a vector of `std::any`.
/// This vector is then dispatched to a polymorphic factory that will try to unpack, match the arguments to its
/// requirements and fail gracefully on error.
struct AnyOperatorFactory {
    virtual ~AnyOperatorFactory() = default;
    template<class... Args>
    AnyOperatorPtr create(Args&&... args) const
    {
        return _create({std::forward<Args>(args)...});
    }

private:
    virtual AnyOperatorPtr _create(std::vector<std::any>&&) const = 0;
};

/// Reactive operator factory implementation.
/// Takes a function as an argument and constructs a calling object around it, that takes care of matching a vector of
/// `std::any` objects to the arguments of the given function.
template<class Func, class traits = notf::function_traits<Func&>>
class ReactiveOperatorFactory : public AnyOperatorFactory {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Tuple representing the required arguments for calling `m_function`.
    using args_tuple = typename traits::args_tuple;

    static_assert(std::is_constructible_v<AnyOperatorPtr, typename traits::return_type>,
                  "A reactive operator factory must return a type convertible to `AnyOperatorPtr`");

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param function     Factory function to construct the reactive operator instance.
    ReactiveOperatorFactory(Func& function) : m_function(function) {}

private:
    template<std::size_t I, class T>
    static void _parse_argument(std::vector<std::any>& source, args_tuple& target)
    {
        try {
            std::get<I>(target) = fuzzy_any_cast<T>(std::move(source[I]));
        }
        catch (const std::bad_any_cast&) {
            NOTF_THROW(ValueError, "Expected type \"{}\", got \"{}\"", type_name<T>(), type_name(source[I].type()));
        }
    }

    template<std::size_t... I>
    static void _parse_arguments(std::index_sequence<I...>, std::vector<std::any>& source, args_tuple& target)
    {
        (_parse_argument<I, typename traits::template arg_type<I>>(source, target), ...);
    }

    AnyOperatorPtr _create(std::vector<std::any>&& args) const final
    {
        if (args.size() != traits::arity) {
            NOTF_THROW(ValueError, "Reactive operator factory failed. Expected {} argument types, got {}",
                       traits::arity, args.size());
        }

        args_tuple typed_arguments;
        _parse_arguments(std::make_index_sequence<traits::arity>{}, args, typed_arguments);
        return std::apply(m_function, std::move(typed_arguments));
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Factory function to construct the reactive operator instance.
    std::function<Func> m_function;
};

// the reactive registry ============================================================================================ //

/// Registy string -> AnyOperatorFactory.
/// Is populated with types at compile time, but can be extended at runtime.
class TheReactiveRegistry {

    // types ----------------------------------------------------------------------------------- //
public:
    template<class T>
    struct register_operator {
    private:
        struct registration {
            registration() { TheReactiveRegistry::_get_registry().emplace(T::name, T::create()); }
        };
        // force the registration at application startup by force instantiating an `exec_register` instance
        template<registration&>
        struct force_instantiation {};
        static inline registration registration_instance;
        static force_instantiation<registration_instance> _;
    };

    /// Map std::string -> reactive operator factory.
    using Registry = std::unordered_map<std::string, std::unique_ptr<AnyOperatorFactory>>;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Creates an untyped reactive operator instance from the registry.
    /// @param name             Name of the operator type.
    /// @param args             Arguments required to instantiate the operator.
    /// @throws OutOfBounds   If the name does not identify an operator type.
    /// @throws ValueError     If any of the arguments do not match the expected type.
    /// @returns                Untyped reactive operator.
    template<class... Args>
    static auto create(const std::string& name, Args&&... args)
    {
        auto& registry = _get_registry();
        auto itr = registry.find(name);
        if (itr == registry.end()) {
            NOTF_THROW(OutOfBounds, "No operator named \"{}\"  in the reactive registry", name);
        }
        return itr->second->create(std::forward<Args>(args)...);
    }

    /// Creates a correctly typed reactive operator instance from the registry.
    /// @param name             Name of the operator type.
    /// @param args             Arguments required to instantiate the operator.
    /// @throws ValueError     If any of the arguments do not match the expected type.
    /// @returns                Correctly typed reactive operator or empty on any failure (wrong name or wrong types).
    template<class I, class O = I, class Policy = detail::DefaultPublisherPolicy, class... Args>
    static std::shared_ptr<Operator<I, O, Policy>> create(const std::string& name, Args&&... args)
    {
        auto& registry = _get_registry();
        auto itr = registry.find(name);
        if (itr == registry.end()) { return {}; }
        return std::dynamic_pointer_cast<Operator<I, O, Policy>>(itr->second->create(std::forward<Args>(args)...));
    }

    /// Checks if the registry knows of an operator type with the given name.
    /// @param name Name of the operator type.
    /// @returns    True, iff the name identifies an operator type.
    static bool has_operator(const std::string& name) { return _get_registry().count(name) == 1; }

private:
    /// Encapsulates a static registry in order to properly initialize it on application start up.
    static Registry& _get_registry()
    {
        static Registry registry;
        return registry;
    }
};

/// Register a *non-template* function that creates an AnyOperatorPtr.
#define NOTF_REGISTER_REACTIVE_OPERATOR(X)                                                         \
    struct Register##X : public TheReactiveRegistry::register_operator<Register##X> {              \
        static constexpr const char* name = #X;                                                    \
        static auto create() { return std::make_unique<decltype(ReactiveOperatorFactory(X))>(X); } \
    }

NOTF_CLOSE_NAMESPACE
