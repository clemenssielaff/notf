#include <iostream>

#include <array>

#include "notf/meta/array.hpp"
#include "notf/meta/concept.hpp"
#include "notf/meta/macros.hpp"

NOTF_USING_NAMESPACE;

static_assert(make_array_of<3>(34)[0] == 34);
static_assert(make_array_of<3>(34)[1] == 34);
static_assert(make_array_of<3>(34)[2] == 34);

struct AnyArithmetic {};

template<class Derived, class Component, size_t Dimensions>
struct Arithmetic : public AnyArithmetic {

    static_assert(Dimensions > 0, "Cannot define a zero-dimensional arithmetic type");
    static_assert(std::disjunction_v<std::is_arithmetic<Component>, std::is_base_of<AnyArithmetic, Component>>,
                  "Component type must be either a scalar or a subclass of Arithmetic");

    // helper ---------------------------------------------------------------------------------- //
private:
    struct _detail {
        template<class T, class = void>
        struct produce_element {
            using type = T;
        };
        template<class T>
        struct produce_element<T, std::void_t<typename T::element_t>> {
            using type = typename T::element_t;
        };
    };

    // types ----------------------------------------------------------------------------------- //
public:
    using component_t = Component;

    using element_t = typename _detail::template produce_element<component_t>::type;

    using Data = std::array<component_t, Dimensions>;

private:
    using derived_t = Derived;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Default constructor.
    constexpr Arithmetic() noexcept = default;

    /// Value constructor.
    /// @param data Raw data for this arithmetic type.
    constexpr Arithmetic(Data data) noexcept : data(std::move(data)) {}

    static constexpr size_t get_dimensions() { return Dimensions; }

    static constexpr size_t get_size() {
        if constexpr (std::is_arithmetic_v<component_t>) {
            return get_dimensions();
        } else {
            return get_dimensions() * component_t::get_size();
        }
    }

    constexpr static derived_t all(const element_t& value) {
        if constexpr (std::is_arithmetic_v<component_t>) {
            return {make_array_of<get_dimensions()>(value)};
        } else {
            return {make_array_of<get_dimensions()>(component_t::all(value))};
        }
    }

    constexpr static derived_t zero() { return {Data{}}; }

    // fields ---------------------------------------------------------------------------------- //
public:
    /// Value data array.
    Data data;
};

struct V2 : public Arithmetic<V2, int, 2> {};
struct M2x3 : public Arithmetic<M2x3, V2, 3> {};

static_assert(V2::zero().data[1] == 0);
static_assert(std::is_same_v<V2::element_t, int>);
static_assert(std::is_same_v<M2x3::element_t, int>);
static_assert(V2::get_size() == 2);
static_assert(M2x3::get_size() == 6);

int main() { return 0; }
