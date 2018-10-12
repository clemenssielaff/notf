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
/// This vector is then dispatched to a polymorphic factory that will try to unpack and match the arguments to its
/// requirements and fail gracefully on error.
struct AnyOperatorFactory {
    virtual ~AnyOperatorFactory() = default;
    template<class... Args>
    std::shared_ptr<AnyOperator> create(Args&&... args) const
    {
        return _create({std::forward<Args>(args)...});
    }

private:
    virtual std::shared_ptr<AnyOperator> _create(std::vector<std::any>&&) const = 0;
};

template<class Func, class traits = notf::function_traits<Func&>>
class ReactiveOperatorFactory : public AnyOperatorFactory {

    // types -------------------------------------------------------------------------------------------------------- //
private:
    using args_tuple = typename traits::args_tuple;

    static_assert(std::is_constructible_v<std::shared_ptr<AnyOperator>, typename traits::return_type>,
                  "A reactive operator factory must return a type convertible to `std::shared_ptr<AnyOperator>`");

    // methods ------------------------------------------------------------------------------------------------------ //
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
            NOTF_THROW(value_error, "Expected type \"{}\", got \"{}\"", type_name<T>(), type_name(source[I].type()));
        }
    }

    template<std::size_t... I>
    static void _parse_arguments(std::index_sequence<I...>, std::vector<std::any>& source, args_tuple& target)
    {
        (_parse_argument<I, typename traits::template arg_type<I>>(source, target), ...);
    }

    std::shared_ptr<AnyOperator> _create(std::vector<std::any>&& args) const final
    {
        if (args.size() != traits::arity) {
            NOTF_THROW(value_error, "Reactive operator factory failed. Expected {} argument types, got {}",
                       traits::arity, args.size());
        }

        args_tuple typed_arguments;
        _parse_arguments(std::make_index_sequence<traits::arity>{}, args, typed_arguments);
        return std::apply(m_function, std::move(typed_arguments));
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Factory function to construct the reactive operator instance.
    std::function<Func> m_function;
};

// the reactive registry ============================================================================================ //

class TheReactiveRegistry {

    // types -------------------------------------------------------------------------------------------------------- //
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

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Creates a reactive operator instance from the registry.
    /// @param name     Name of the operator type.
    /// @param args     Arguments required to instantiate the operator.
    template<class... Args>
    static auto create(const std::string& name, Args&&... args)
    {
        auto& registry = _get_registry();
        auto itr = registry.find(name);
        if (itr == registry.end()) {
            NOTF_THROW(out_of_bounds, "No operator named \"{}\"  in the reactive registry", name);
        }
        return itr->second->create(std::forward<Args>(args)...);
    }

private:
    /// Encapsulates a static registry in order to properly initialize it on application start up.
    static Registry& _get_registry()
    {
        static Registry registry;
        return registry;
    }
};

/// Register a *non-template* function that creates a std::shared_ptr<>
#define NOTF_REGISTER_REACTIVE_OPERATOR(X)                                                         \
    struct Register##X : public TheReactiveRegistry::register_operator<Register##X> {              \
        static constexpr const char* name = #X;                                                    \
        static auto create() { return std::make_unique<decltype(ReactiveOperatorFactory(X))>(X); } \
    }

NOTF_CLOSE_NAMESPACE
