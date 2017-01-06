#pragma once

#include <map>
#include <memory>
#include <string>

#include "common/signal.hpp"
#include "utils/macro_overload.hpp"

namespace notf {

class AbstractProperty;

using PropertyMap = std::map<std::string, std::unique_ptr<AbstractProperty>>;

/**********************************************************************************************************************/

/** An abstract Property.
 * Is pretty useless by itself, you'll have to cast it to a subclass of Property<T> to get any functionality out of it.
 */
class AbstractProperty : public receive_signals<class AbstractProperty> {

public: // methods
    explicit AbstractProperty(const PropertyMap::iterator iterator)
        : m_it(iterator)
    {
    }

    /** The name of this Property. */
    const std::string& get_name() const { return m_it->first; }

private: // fields
    /** Iterator to the item in the PropertyMap containing this Property.
     * Is valid as long as this Property exists.
     * Storing this is a lot cheaper than storing the name again.
     */
    const PropertyMap::iterator m_it;
};

/**********************************************************************************************************************/

/** A (templated) Property baseclass, used to derive subclasses in bulk (see property_impl.hpp for details). */
template <typename T>
class Property : public AbstractProperty {

public: // methods
    Property(const T value, const PropertyMap::iterator iterator)
        : AbstractProperty(std::move(iterator))
        , m_value(std::move(value))
        , m_expression()
    {
    }

    /** Returns the current value of this Property. */
    const T& get_value() const { return m_value; }

    /** Updates the value of this Property.
     * If the Property is defined through an expression, manually setting the value will remove the expression.
     */
    void set_value(T value)
    {
        m_value = std::move(value);
        m_expression = nullptr;
        value_changed(m_value);
    }

    /** Assigns a new expression to this Property.
     * If the given expression is not empty, it is executed immediately and its result stored in the Property's value.
     */
    void _set_expression(std::function<T()> expression)
    {
        using std::swap;
        std::swap(m_expression, expression);
        _update_expression();
    }

    /** Updates the value of this Property through its expression.
     * Is ignored, if no expression is set.
     */
    void _update_expression()
    {
        if (m_expression) {
            m_value = m_expression();
            value_changed(m_value);
        }
    }

public: // signals
    /** Emitted when the value of this Property has changed. */
    Signal<const T&> value_changed;

private: // fields
    /** Value of this Property. */
    T m_value;

    /** Expression defining this Property (can be empty). */
    std::function<T()> m_expression;
};

/**********************************************************************************************************************/

/** Adds a new Property to the given PropertyMap.
 * @param property_map  Map to which to add the Property.
 * @param name          Name of the Property, must be unique in the map.
 * @param value         Value of the Property, must be of a type supported by AbstractProperty.
 * @return              Iterator to the new Property in the map.
 * @throw               std::runtime_error if the name is not unique.
 */
template <typename T>
PropertyMap::iterator add_property(PropertyMap& property_map, std::string name, const T value);

/**********************************************************************************************************************/

namespace detail {

    template <typename TARGET>
    constexpr void connect_property_signals(Property<TARGET>&)
    {
        // stop recursion
    }

    template <typename TARGET, typename T, typename... ARGS,
              std::enable_if_t<!std::is_base_of<AbstractProperty, std::decay_t<T>>::value, std::nullptr_t> = nullptr>
    constexpr void connect_property_signals(Property<TARGET>&, T&&, ARGS&&... args)
    {
        // ignore non-Property `T`s
        return connect_property_signals(std::forward<ARGS>(args)...);
    }

    template <typename TARGET, typename T, typename... ARGS,
              std::enable_if_t<std::is_base_of<AbstractProperty, std::decay_t<T>>::value, std::nullptr_t> = nullptr>
    constexpr void connect_property_signals(Property<TARGET>& target, Property<T>& property, ARGS&&... args)
    {
        // connect the value_changed signal of all Property arguments
        target.connect_signal(property.value_changed, &Property<TARGET>::_update_expression);
        return connect_property_signals(std::forward<ARGS>(args)...);
    }

} // namespace detail

#define _notf_variadic_capture(...) NOTF_OVERLOADED_MACRO(_notf_variadic_capture, __VA_ARGS__)
#define _notf_variadic_capture1(a) &#a = a
#define _notf_variadic_capture2(a, b) &a = a, &b = b
#define _notf_variadic_capture3(a, b, c) &#a = a, &#b = b, &#c = c
#define _notf_variadic_capture4(a, b, c, d) &#a = a, &#b = b, &#c = c, &#d = d
#define _notf_variadic_capture5(a, b, c, d, e) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e
#define _notf_variadic_capture6(a, b, c, d, e, f) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f
#define _notf_variadic_capture7(a, b, c, d, e, f, g) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g
#define _notf_variadic_capture8(a, b, c, d, e, f, g, h) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h
#define _notf_variadic_capture9(a, b, c, d, e, f, g, h, i) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i
#define _notf_variadic_capture10(a, b, c, d, e, f, g, h, i, j) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j
#define _notf_variadic_capture11(a, b, c, d, e, f, g, h, i, j, k) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j, &#k = k
#define _notf_variadic_capture12(a, b, c, d, e, f, g, h, i, j, k, l) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j, &#k = k, &#l = l
#define _notf_variadic_capture13(a, b, c, d, e, f, g, h, i, j, k, l, m) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j, &#k = k, &#l = l, &#m = m
#define _notf_variadic_capture14(a, b, c, d, e, f, g, h, i, j, k, l, m, n) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j, &#k = k, &#l = l, &#m = m, &#n = n
#define _notf_variadic_capture15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j, &#k = k, &#l = l, &#m = m, &#n = n, &#o = o
#define _notf_variadic_capture16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) &#a = a, &#b = b, &#c = c, &#d = d, &#e = e, &#f = f, &#g = g, &#h = h, &#i = i, &#j = j, &#k = k, &#l = l, &#m = m, &#n = n, &#o = o, &#p = p

#define define_expression(TARGET, LAMBDA, ...)                              \
    TARGET._set_expression([_notf_variadic_capture(__VA_ARGS__)]()LAMBDA); \
    notf::detail::connect_property_signals(__VA_ARGS__)

} // namespace notf
