#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include "common/signal.hpp"
#include "utils/macro_overload.hpp"

namespace notf {

class AbstractProperty;
template <typename>
class Property;

/**********************************************************************************************************************/

namespace detail {

    /** Access for function to create a Property expression (see `PropertyExpression`-macro below). */
    template <typename TYPE, typename EXPR, typename... ARGS>
    void create_property_expression(Property<TYPE>* property, EXPR&& expression, ARGS&&... dependencies)
    {
        // order is important here, we need the dependencies in place when the expression is assigned and evaluated
        property->_drop_expression();
        property->_add_dependencies(std::forward<ARGS>(dependencies)...);
        property->_set_expression(std::forward<EXPR>(expression));
    }

} // namespace detail

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

protected: // methods
    explicit AbstractProperty(const PropertyMap::iterator iterator)
        : m_it(iterator)
    {
    }

public: // methods
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
 * To ensure that if a Property expression compiles, it also works, use the `PropertyExpression` macro.
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

    template <typename TYPE, typename EXPR, typename... ARGS>
    friend void detail::create_property_expression(Property<TYPE>*, EXPR&&, ARGS&&...);

protected: // methods
    Property(const T value, const PropertyMap::iterator iterator)
        : AbstractProperty(std::move(iterator))
        , m_value(std::move(value))
        , m_is_dirty(false)
        , m_expression()
        , m_dependencies()
    {
    }

public: // methods
    virtual ~Property() override { on_deletion(); }

    /** Returns the current value of this Property. */
    const T& get_value() const
    {
        _make_clean();
        return m_value;
    }

    /** Tests whether this Property is currently defined by an Expression. */
    bool has_expression() const { return static_cast<bool>(m_expression); }

protected: // methods
    /** Updates the value of this Property.
     * If the Property is defined through an expression, manually setting the value will remove the expression.
     */
    void _set_value(const T value)
    {
        _drop_expression();
        _change_value(std::move(value));
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

private: // methods
    /** Assigns a new expression to this Property and executes it immediately. */
    void _set_expression(std::function<T()> expression)
    {
        m_expression = expression;
        _change_value(m_expression());
    }

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
    void _make_clean() const
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

    /** Adds a new dependency to this Property.
     * Every time a dependency Property is updated, this Property will try to re-evaluate its expression.
     * Always make sure that all Properties that this Property's expression depends on are registered as dependencies.
     * Existing dependencies are ignored.
     */
    template <typename OTHER>
    void _add_dependency(Property<OTHER>* dependency)
    {
        { // make sure that each dependency is unique
            AbstractProperty* abstract_dependency = static_cast<AbstractProperty*>(dependency);
            if (m_dependencies.count(abstract_dependency) == 0) {
                m_dependencies.insert(abstract_dependency);
            }
        }

        // connect all relevant signals
        connect_signal(dependency->_signal_test, [this]() { this->_signal_test(); });
        connect_signal(dependency->_signal_dirty, &Property::_make_dirty);
        connect_signal(dependency->_signal_clean, &Property::_make_clean);
        connect_signal(dependency->on_deletion, &Property::_drop_expression);
    }

    /** Takes any kind of parameter list, extracts only the Properties and adds dependencies to each one of them. */
    template <typename TYPE, typename... ARGS>
    void _add_dependencies(Property<TYPE>* dependency, ARGS&&... args)
    {
        _add_dependency(dependency);
        return _add_dependencies(std::forward<ARGS>(args)...);
    }
    void _add_dependencies() {} // stop recursion

private: // fields
    /** Value of this Property. */
    mutable T m_value;

    /** Dirty flag, used to avoid redundant expression evaluations. */
    mutable bool m_is_dirty;

    /** Expression defining this Property (can be empty). */
    std::function<T()> m_expression;

    /** All Properties that this one dependends on through its expression. */
    std::set<AbstractProperty*> m_dependencies;
};

/**********************************************************************************************************************/

} // namespace notf

/** Convenience macro to create a Property expression lambda and add all of its dependencies in one go. */
#define PropertyExpression(TARGET, LAMBDA, ...) \
    notf::detail::create_property_expression(TARGET, [__VA_ARGS__] LAMBDA, __VA_ARGS__)

/* TODO: Property overhaul (hopefully for the last time).
 * There are a few things that I want.
 * 1.   Make `Property` a private class.
 *      This is de-facto the case already, with a protected Constructor and an argument that can only be supplied when
 *      you have access to a PropertyMap.
 *      But should we move it into the "detail" namespace in favor of a notf::Property class which is the accessor?
 * 2.   The PropertyMap should return not a raw pointer to a Property, but a Property accessor.
 *      The accessor can be assigned a `T` and an Expression via the `=` operator.
 *      It can also be dereferenced via '*' operator to return a const T&.
 *      Internally, it is just a wrapper for a raw Property pointer.
 *      Expression lambdas take Property accessors as captures.
 * 3.   Expressions.
 *      There should be a type of expressions that can be assigned to a Property accessor via the `=` operator.
 * 4.   Can we get rid of some of the macros in properties.hpp?
 * 5.   Inside a Property expression, the properties should be unpacked to their values.
 *      What I mean is that I want to write:
 *          PropertyExpression(
 *              target,
 *              {
 *                  return a + b;
 *              },
 *              a, b);
 *      and not the same code with "a->get_value() + b->get_value()".
 */
