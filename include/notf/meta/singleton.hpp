#pragma once

#include <atomic>
#include <optional>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/typename.hpp"

NOTF_OPEN_NAMESPACE

// scoped singleton ================================================================================================= //

/// Error thrown when you try to access an uninitialized Singleton or try to initialize it more than once.
NOTF_EXCEPTION_TYPE(SingletonError);

/// One issue with static Singletons is that their destruction order is essentially random.
/// This can lead to problems if a Singleton needs another one to destruct correctly as it will fail sometimes, and
/// sometimes it won't.
/// The ScopedSingleton is a small wrapper around a type `T` of which only one instance should be alive at any given
/// point during the application's runtime. The first instance of the ScopedSingleton initializes the static instance of
/// `T` and ultimately destroys it during its own destruction.
/// The instance of ScopedSingleton that initialized the static instance `T` is called the holder. We need to
/// differentiate the holder and all other instances because only the holder may destruct the static instance of `T`.
/// You can always create instances of `ScopedSingleton<T>` without any arguments, which will create an "access-only"
/// instance, that will never try to become the holder ... Unless `T` is default-constructible. In that case, it is
/// possible that an instance that was meant as an access-only instance suddenly becomes the holder, if it is the first
/// one to be called. If there is any doubt, you can use `is_holder` to check at runtime or pass `NoHolder` as the
/// single constructor argument.
/// If you try to access the static instance of T without a valid instance of ScopedSingleton around, it will throw a
/// `SingletonError`.
template<class T>
class ScopedSingleton {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Type stored in this ScopedSingleton.
    using type = T;

    /// Trait passed in as the first argument for a holding ScopedSingleton.
    /// Is used to differentiate between an access-singleton (default constructible) and one that constructs a default-
    /// constructible type T to hold.
    struct Holder {};

protected:
    /// State of the Singleton.
    enum class _State {
        EMPTY,
        INITIALIZING,
        RUNNING,
        DESTROYING,
    };

    // methods --------------------------------------------------------------------------------- //
protected:
    /// Perfect-forwarding constructor.
    /// @param args             All arguments passed to the constructor of `T`.
    /// @throws SingletonError  If you try to instantiate more than one instance of `ScopedSingleton<T>`.
    template<class... Args>
    ScopedSingleton(Holder, Args&&... args) {
        if (_State expected = _State::EMPTY; s_state.compare_exchange_strong(expected, _State::INITIALIZING)) {
            s_instance.emplace(std::forward<Args>(args)...);
            s_state.store(_State::RUNNING);
            m_is_holder = true;
        } else {
            NOTF_THROW(SingletonError, "Cannot create more than once instance of type ScopedSingleton<{}>",
                       type_name<T>());
        }
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(ScopedSingleton);

    /// Default constructor. Never attempts to create the static instance of `T`.
    ScopedSingleton() = default;

    /// Destructor.
    /// If this instance is the holder, the destructor destroys the static instance of `T` and resets the
    /// ScopedSingleton state so it can be instantiated anew.
    ~ScopedSingleton() noexcept(std::is_nothrow_destructible_v<T>) {
        if (m_is_holder) {
            if (_State expected = _State::RUNNING; s_state.compare_exchange_strong(expected, _State::DESTROYING)) {
                s_instance.reset();
            }
            s_state.store(_State::EMPTY);
        }
    }

    /// Whether or not this instance is the one determining the lifetime of the static instance of `T`.
    bool is_holder() const noexcept { return m_is_holder; }

    /// @{
    /// Access operators.
    T* operator->() { return &_get(); }
    T& operator*() { return _get(); }
    const T* operator->() const { return &_get(); }
    const T& operator*() const { return _get(); }
    /// @}

protected:
    /// The static instance of `T`.
    /// If no `ScopedSingleton<T>` instance is alife, this throws a SingletonError.
    static T& _get() {
        if (NOTF_LIKELY(s_state == _State::RUNNING)) { return s_instance.value(); }
        NOTF_THROW(SingletonError, "No instance of ScopedSingleton<{}> exists", type_name<T>());
    }

    /// The current state of the Singleton.
    static _State _get_state() noexcept { return s_state; }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Whether or not this instance is the one determining the lifetime of the static instance of `T`.
    bool m_is_holder = false;

    /// EMPTY -> INITIALIZING -> RUNNING -> DESTROYING -> EMPTY
    inline static std::atomic<_State> s_state = _State::EMPTY;

    /// The static instance of `T`.
    inline static std::optional<T> s_instance;
};

NOTF_CLOSE_NAMESPACE
