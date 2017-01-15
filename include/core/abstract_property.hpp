#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include "common/signal.hpp"
#include "utils/macro_overload.hpp"

namespace notf {

class AbstractProperty;

/**********************************************************************************************************************/

/** Special map providing a templated method to create Properties.
 * Subclassing std containers is generally frowned upon but should be okay in this case, since we don't add any fields.
 * See http://stackoverflow.com/a/2034936/3444217
 */
class PropertyMap : public std::map<std::string, std::unique_ptr<AbstractProperty>> {

public: // methods
    /** Adds a new Property.
     * @param name          Name of the Property, must be unique in the map.
     * @param value         Value of the Property, must be of a type supported by AbstractProperty.
     * @return              Pointer to the correct subtype of the new Property in the map.
     * @throw               std::runtime_error if the name is not unique.
     */
    template <typename NAME, typename TYPE>
    NAME* create_property(std::string name, const TYPE value);

    /** Returns a Property by name and type.
     * @param name          Name of the requested Property.
     * @throw               std::out_of_range if the name is not known / std::runtime_error if the type does not match.
     */
    template <typename PROPERTY_TYPE,
              typename = std::enable_if_t<std::is_base_of<AbstractProperty, PROPERTY_TYPE>::value>>
    PROPERTY_TYPE* get(const std::string& name) const;
};

/**********************************************************************************************************************/

/** An abstract Property.
 * Is pretty useless by itself, you'll have to cast it to a subclass of Property<T> to get any functionality out of it.
 */
class AbstractProperty : public receive_signals {

public: // methods
    explicit AbstractProperty(const PropertyMap::iterator iterator)
        : m_it(iterator)
    {
    }

    virtual ~AbstractProperty();

    /** The name of this Property. */
    const std::string& get_name() const { return m_it->first; }

    /** The printable type of this Property. */
    virtual const std::string& get_type() const = 0;

private: // fields
    /** Iterator to the item in the PropertyMap containing this Property.
     * Is valid as long as this Property exists.
     * Storing this is a lot cheaper than storing the name again.
     */
    const PropertyMap::iterator m_it;
};

/**********************************************************************************************************************/

/** Property implementation template.
 * See property.hpp for details for actual subclasses.
 *
 * When setting a Property expression is is mandatory to add all dependencies (Properties that are used within that
 * expression) as well.
 * Sadly, there is no way to automatically find all Properties used in an expression, at least not one that is
 * guaranteed to work in all circumstances.
 * For simple, lambda-based expressions, you can use the `property_expression` macro, but you need to explicitly list
 * all dependency Properties as well for it to work.
 * No capturing [this], if you then use it to access individual member Properties or other high jinks!
 *
 * NoTFs property model is extremely simple and lightweight with only the bare minimum of "magic".
 * You can define expressions for a Property to be dependent on another Property, but trying to create any kind of
 * cyclic evaluation will raise a runtime error.
 * That means `F = m * a` on it's own is okay, but adding `m = F / a' will cause an error since setting `a` will cause
 * `F` to update, which updates `m`, which causes `F` to update again etc.
 * This is not a constraint solver!
 *
 * Creating a dependency cycle raise an error as soon as its expression is evaluated during the initial setup.
 * To make sure that all Properties values are not affected before the dependency cycle is caught, a test signal is
 * fired before each user-initiated value change.
 *
 * Property expressions guarantee that each expression is evaluated only once for each user-initiated change.
 * This includes scenarios, in which a Property is dependent on multiple other Properties that are each dependent on the
 * fourth Property changed by the user.
 * Without that guarantee, the expression of the first property would be evaluated multiple times (once for each of its
 * dependencies).
 */
template <typename T>
class Property : public AbstractProperty {

    template <typename>
    friend class Property;

public: // methods
    Property(const T value, const PropertyMap::iterator iterator)
        : AbstractProperty(std::move(iterator))
        , m_value(std::move(value))
        , m_is_dirty(false)
        , m_expression()
        , m_dependencies()
    {
    }

    virtual ~Property() override { on_deletion(); }

    /** Returns the current value of this Property. */
    const T& get_value()
    {
        _make_clean();
        return m_value;
    }

    /** Tests whether this Property is currently defined by an Expression. */
    bool has_expression() const { return static_cast<bool>(m_expression); }

    /** Updates the value of this Property.
     * If the Property is defined through an expression, manually setting the value will remove the expression.
     */
    void set_value(const T value)
    {
        _drop_expression();
        _change_value(std::move(value));
    }

    /** Assigns a new expression to this Property and executes it immediately. */
    void set_expression(std::function<T()> expression)
    {
        _drop_expression();
        m_expression = expression;
        _change_value(m_expression());
    }

    /** Adds a new dependency to this Property.
     * Every time a dependency Property is updated, this Property will try to re-evaluate its expression.
     * Always make sure that all Properties that this Property's expression depends on are registered as dependencies.
     * Existing dependencies are ignored.
     * @return  True if a dependency was added, false if it is already known.
     */
    template <typename OTHER>
    bool add_dependency(Property<OTHER>* dependency)
    {
        { // make sure that each dependency is unique
            AbstractProperty* abstract_dependency = static_cast<AbstractProperty*>(dependency);
            if (m_dependencies.count(abstract_dependency) == 0) {
                m_dependencies.insert(abstract_dependency);
            }
            else {
                return false;
            }
        }

        // connect all relevant signals
        connect_signal(dependency->_signal_test, [this]() { this->_signal_test(); });
        connect_signal(dependency->_signal_dirty, &Property::_make_dirty);
        connect_signal(dependency->_signal_clean, &Property::_make_clean);
        connect_signal(dependency->on_deletion, &Property::_drop_expression);
        return true;
    }

    /** Takes any kind of parameter list, extracts only the Properties and adds dependencies to each one of them.
     * @return  Number of recognized, unique dependencies.
     */
    template <typename... ARGS>
    uint add_dependencies(ARGS&&... args)
    {
        uint count = 0;
        _extract_dependencies(count, std::forward<ARGS>(args)...);
        return count;
    }

public: // signals
    /** Emitted when the value of this Property has changed. */
    Signal<> value_changed;

    /** Emitted, when the Property is being deleted. */
    Signal<> on_deletion;

private: // signals
    /** Test signal emitted first to check for cyclic dependencies without changing any state. */
    Signal<> _signal_test;

    /** Emitted, when the Property's value was changed by the user (directly or indirectly). */
    Signal<> _signal_dirty;

    /** Emitted, right after `_signal_dirty` signalling all dependent Properties to clean up. */
    Signal<> _signal_clean;

private: // methods for property_expression
    /** Called when the user requests a change of this Property's value. */
    void _change_value(const T value)
    {
        if (value != m_value) {
            m_value = value;
            _signal_test();
            _signal_dirty();
            _signal_clean();
            value_changed();
        }
    }

    /** Dirty propagation callback. */
    void _make_dirty()
    {
        if (!m_is_dirty && has_expression()) {
            m_is_dirty = true;
            _signal_dirty();
        }
    }

    /** Updates the value of this Property through its expression. */
    void _make_clean()
    {
        if (m_is_dirty) {
            assert(has_expression());
            const auto cache = m_value;
            m_value = m_expression();
            m_is_dirty = false;
            _signal_clean();
            if (cache != m_value) {
                value_changed();
            }
        }
    }

    /** Removes the current expression defining this Property without modifying its value. */
    void _drop_expression()
    {
        assert(!m_is_dirty);
        if (has_expression()) {
            m_expression = nullptr;
            disconnect_all_connections();
            m_dependencies.clear();
        }
    }

    /** Private overloaded template function to extract Property dependencies from a mixed types parameter list. */
    template <typename TYPE, typename... ARGS>
    void _extract_dependencies(uint& count, Property<TYPE>* dependency, ARGS&&... args)
    {
        if (add_dependency(dependency)) {
            ++count;
        }
        return _extract_dependencies(count, std::forward<ARGS>(args)...);
    }
    template <typename ARG, typename... ARGS,
              typename = std::enable_if_t<!std::is_base_of<AbstractProperty, std::remove_pointer_t<std::decay_t<ARG>>>::value>>
    void _extract_dependencies(uint& count, ARG&&, ARGS&&... args)
    {
        return _extract_dependencies(count, std::forward<ARGS>(args)...); // ignore non-Property `ARG`
    }
    void _extract_dependencies(uint&) {} // stop recursion

private: // fields
    /** Value of this Property. */
    T m_value;

    /** Dirty flag, used to avoid redundant expression evaluations. */
    bool m_is_dirty;

    /** Expression defining this Property (can be empty). */
    std::function<T()> m_expression;

    /** All Properties that this one dependends on through its expression. */
    std::set<AbstractProperty*> m_dependencies;
};

/**********************************************************************************************************************/

#define _notf_variadic_capture(...) NOTF_OVERLOADED_MACRO(_notf_variadic_capture, __VA_ARGS__)
#define _notf_variadic_capture1(a) &a = a
#define _notf_variadic_capture2(a, b) &a = a, &b = b
#define _notf_variadic_capture3(a, b, c) &a = a, &b = b, &c = c
#define _notf_variadic_capture4(a, b, c, d) &a = a, &b = b, &c = c, &d = d
#define _notf_variadic_capture5(a, b, c, d, e) &a = a, &b = b, &c = c, &d = d, &e = e
#define _notf_variadic_capture6(a, b, c, d, e, f) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f
#define _notf_variadic_capture7(a, b, c, d, e, f, g) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g
#define _notf_variadic_capture8(a, b, c, d, e, f, g, h) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h
#define _notf_variadic_capture9(a, b, c, d, e, f, g, h, i) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i
#define _notf_variadic_capture10(a, b, c, d, e, f, g, h, i, j) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j
#define _notf_variadic_capture11(a, b, c, d, e, f, g, h, i, j, k) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j, &k = k
#define _notf_variadic_capture12(a, b, c, d, e, f, g, h, i, j, k, l) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j, &k = k, &l = l
#define _notf_variadic_capture13(a, b, c, d, e, f, g, h, i, j, k, l, m) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j, &k = k, &l = l, &m = m
#define _notf_variadic_capture14(a, b, c, d, e, f, g, h, i, j, k, l, m, n) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j, &k = k, &l = l, &m = m, &n = n
#define _notf_variadic_capture15(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j, &k = k, &l = l, &m = m, &n = n, &o = o
#define _notf_variadic_capture16(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) &a = a, &b = b, &c = c, &d = d, &e = e, &f = f, &g = g, &h = h, &i = i, &j = j, &k = k, &l = l, &m = m, &n = n, &o = o, &p = p

/** Convenience macro to create a Property expression lambda and add all of its dependencies in one go.
 * It is about the same amount of code as if you were to write it out by hand.
 * However, failing to include an explicit dependency in the captures list does cause a compiler error, which helps with
 * finding bugs early one.
 * You can use the macro's return value to check the number of recognized, unique dependencies.
 */
#define property_expression(TARGET, LAMBDA, ...) \
    TARGET->add_dependencies(__VA_ARGS__);       \
    TARGET->set_expression([_notf_variadic_capture(__VA_ARGS__)] LAMBDA)

} // namespace notf
