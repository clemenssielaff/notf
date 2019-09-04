#include <exception>
#include <type_traits>

#include "notf/common/optional.hpp"
#include "notf/common/vector.hpp"

#include "notf/meta/function.hpp"
#include "notf/meta/tuple.hpp"

using namespace notf;

struct SKIP {};
struct DONE {};

template<class T>
struct Passthrough {
    std::optional<T> operator()(T value) noexcept { return {std::move(value)}; }
};

template<class T>
struct AddOne {
    std::optional<T> operator()(T value) noexcept { return {std::move(value) + 1}; }
};

template<class T>
struct MakePair {
    std::optional<std::pair<T, T>> operator()(T value) noexcept {
        if (prev.has_value()) {
            std::pair<T, T> result = std::make_pair(std::move(prev.value()), std::move(value));
            prev.reset();
            return {std::move(result)};
        } else {
            prev = std::move(value);
            return {};
        }
    }

private:
    std::optional<T> prev = {};
};

class Observer : public std::enable_shared_from_this<Observer> {

protected:
    Observer() = default;

public:
    virtual ~Observer() = default;
    virtual void on_error(const std::exception_ptr& /*exception*/) {}
    virtual void on_complete() {}

    template<class T, class ...Args, class = std::enable_if_t<std::is_base_of_v<Observer, T>>>
    static std::shared_ptr<T> create(Args&&...args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
};

template<class T>
struct TypedObserver : public Observer {
    virtual void on_next(const T& value) = 0;
};


template<class... Ops>
class Observable {
    static constexpr const std::size_t op_count = sizeof...(Ops);
    static_assert(op_count > 0, "Cannot create an Observable without a single operation");

    using Operations = std::tuple<Ops...>;

    using first_op_traits = function_traits<std::tuple_element_t<0, Operations>>;
    static_assert(first_op_traits::arity == 1);
    using input_t = typename first_op_traits::template arg_type<0>;

    using last_op_traits = function_traits<std::tuple_element_t<op_count - 1, Operations>>;
    using output_t = typename last_op_traits::return_type;
    static_assert(is_optional_v<output_t>, "Operators must return a std::optional<T> value");

    struct IsNotEmptyFunctor {
        template<class T>
        constexpr bool operator()() noexcept {
            return !std::is_empty_v<T>; // return true iff T is not an empty type
        }
    };
    using Data = create_filtered_tuple<IsNotEmptyFunctor, Ops...>;

    // methods --------------------------------------------------------------------------------- //
public:
    output_t operator()(const input_t& value) {
        try {
            return _call_next<0>(value);
        }
        catch (...) {
            _error(std::current_exception());
            return {};
        }
    }

protected:
    void _error(std::exception_ptr /*exception*/) {
        // TODO: _on_error
    }

    void _complete() {
        // TODO: _on_complete
    }

private:
    template<std::size_t I, class T>
    auto _call_next(T value) {
        if constexpr (I < op_count) {
            if (auto result = _call_next_operator<I>(std::move(value))) {
                static_assert(is_optional_v<decltype(result)>, "Operators must return a std::optional<T> value");
                return _call_next<I + 1>(std::move(result.value()));
            } else {
                return output_t{};
            }

        } else {
            return output_t(std::move(value));
        }
    }

    template<std::size_t I, class T>
    auto _call_next_operator(T value) {
        using Operation = std::tuple_element_t<I, Operations>;
        if constexpr (std::is_empty_v<Operation>) {
            return Operation()(std::move(value));
        } else {
            constexpr const std::size_t data_index = get_filtered_tuple_index<IsNotEmptyFunctor, I, Operations>();
            return std::get<data_index>(m_data)(std::move(value));
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    std::vector<std::weak_ptr<TypedObserver<output_t>>> m_observers;

    /// Operation data.
    Data m_data;
};

int main() {
    Observable<Passthrough<int>, Passthrough<int>, Passthrough<int>, Passthrough<int>, Passthrough<int>,
               Passthrough<int>, AddOne<int>, Passthrough<int>, MakePair<int>>
        op;
    op(1);
    std::pair<int, int> val = op(1).value();
    const int return_value = val.first + val.second;
    return return_value;
    //    return sizeof(op);
}
