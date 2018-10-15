#pragma once

#include "notf/meta/hash.hpp"
#include "notf/meta/log.hpp"
#include "notf/reactive/reactive_operator.hpp"

#include "./graph.hpp"

NOTF_OPEN_NAMESPACE

// property ========================================================================================================= //

struct AnyProperty {
    NOTF_NO_COPY_OR_ASSIGN(AnyProperty);
    AnyProperty() = default;
    virtual ~AnyProperty() = default;
};

template<class T>
struct Property : public AnyProperty, public Operator<T, T, detail::MultiPublisherPolicy> {

    static_assert(std::is_move_constructible_v<T>, "Property values must be movable to freeze them");
    static_assert(is_hashable_v<T>, "Property values must be hashable");
    static_assert(std::is_const_v<T>, "Property values must be modifyable");
    static_assert(!std::is_same_v<T, None>, "Property value type must not be None");

    // types -------------------------------------------------------------------------------------------------------- //
public:
    using type_t = T;

private:
    using operator_t = Operator<type_t, type_t, detail::MultiPublisherPolicy>;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Value constructor.
    /// @param value        Property value.
    Property(T value, bool is_visible = true) : m_value(std::move(value)), m_is_visible(is_visible) {}

    ~Property() override
    {
        _clear_frozen();
        complete();
    }

    /// The Node-unique name of this Property.
    virtual std::string_view get_name() const = 0;

    /// The Property value.
    constexpr const T& get() const noexcept { return _get(std::this_thread::get_id()); }

    /// Updater the Property value and bind it.
    /// @param value    New Property value.
    constexpr void set(const T& value)
    {
        set_bound();
        _set(value);
    }

    /// Whether a change in the Property will cause the Node to redraw or not.
    constexpr bool is_visible() const noexcept { return m_is_visible; }

    /// Updates the visibility flag of this Property.
    void set_is_visible(bool is_visible) noexcept { m_is_visible = is_visible; }
    void set_visible() noexcept { m_is_visible = true; }
    void set_invisible() noexcept { m_is_visible = false; }

    /// Tests whether this Propety is bound or free.
    /// A bound Property will no accept updates pushed from a reactive upstream.
    bool is_bound() const noexcept { return m_is_bound; }

    /// Updates the bound/free flag of this Property.
    void set_is_bound(bool is_bound) noexcept { m_is_bound = is_bound; }
    void set_bound() noexcept { m_is_bound = true; }
    void set_free() noexcept { m_is_bound = false; }

private:
    /// Current NodeProperty value.
    /// @param thread_id    Id of this thread. Is exposed so it can be overridden by tests.
    const T& _get(const std::thread::id thread_id) const
    {
        // if the property is frozen by this thread (the render thread, presumably) and there exists a frozen copy of
        // the value, use that instead of the current one
        if (TheGraph::is_frozen_by(thread_id)) {
            if (T* frozen_value = m_frozen_value.load(std::memory_order_consume)) { return *frozen_value; }
        }
        return m_value;
    }

    /// Updates the value of the Property.
    /// @param value    New value.
    void _set(const T& value)
    {
        // do nothing if the property value would not actually change
        if (value == m_value) { return; }

        // if the property is currently frozen and this is the first modification, create a frozen copy of the current
        // value before changing it
        if (TheGraph::is_frozen()) {
            if (T* frozen_value = m_frozen_value.load(std::memory_order_relaxed); !frozen_value) {
                frozen_value = new T(std::move(m_value));
                m_frozen_value.store(frozen_value, std::memory_order_release);
            }
        }

        // update and publish
        m_value = value;
        publish(m_value);
    }

    /// Deletes the frozen value copy, if one exists.
    void _clear_frozen()
    {
        if (T* frozen_ptr = m_frozen_value.exchange(nullptr)) { delete frozen_ptr; }
    }

    // reactive methods --------------------------------------------------------------------------------------------- //
private:
    void on_next(const AnyPublisher* /*publisher*/, const T& value) final
    {
        if (m_is_bound) {
            NOTF_LOG_WARN("Failed to change *bound* property \"{}\" from '{}' to '{}'", get_name(), m_value, value);
            return;
        }
        _set(value);
    }

    void on_error(const AnyPublisher* /*publisher*/, const std::exception& /*exception*/) final
    {
        // TODO: how does a Property handle upstream errors? It should swallow it and not pass it on downstream, but
        // do we just report it? re-throw?
    }

    void on_complete(const AnyPublisher* /*publisher*/) final {} // Properties cannot be completed from the outside

    using operator_t::complete; // don't complete the Property while it is alive
    using operator_t::error;    // don't propagate errors manually
    using operator_t::publish;  // don't publish values manually

    // members ------------------------------------------------------------------------------------------------------ //
protected:
    /// The Property value.
    type_t m_value;

    /// Pointer to a frozen copy of the value, if it was modified while the Graph was frozen.
    std::atomic<T*> m_frozen_value{nullptr};

    /// Whether a change in the Property will cause the Node to redraw or not.
    bool m_is_visible;

    /// A bound Property will no accept updates pushed from a reactive upstream.
    bool m_is_bound = false;
};

// property handle ================================================================================================== //

NOTF_CLOSE_NAMESPACE
