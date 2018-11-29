#include <atomic>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/typename.hpp"

NOTF_USING_NAMESPACE;

/// Error thrown when you try to access an uninitialized Singleton.
NOTF_EXCEPTION_TYPE(SingletonError);

template<class T>
class ScopedSingleton {

    // types ----------------------------------------------------------------------------------- //
private:
    /// State of the Singleton.
    enum class State {
        EMPTY,
        INITIALIZING,
        RUNNING,
        DESTROYING,
    };

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(ScopedSingleton);

    /// Default constructor. Never attempts to create the static instance of `T`.
    template<class X = T, class = std::enable_if_t<!std::is_default_constructible_v<X>>>
    ScopedSingleton() {}

    /// Perfect-forwarding constructor.
    /// @param args             All arguments passed to the constructor of `T`.
    /// @throws SingletonError  If you try to instantiate more than one instance of `ScopedSingleton<T>`.
    template<class... Args>
    ScopedSingleton(Args&&... args) {
        if (State expected = State::EMPTY; s_state.compare_exchange_strong(expected, State::INITIALIZING)) {
            s_instance = T(std::forward<Args>(args)...);
            s_state.store(State::RUNNING);
            m_is_holder = true;
            std::cout << "initializing" << std::endl;
        }
    }

    /// Destructor.
    /// If this instance is the holder, the destructor destroys the static instance of `T` and resets the
    /// ScopedSingleton state so it can be instantiated anew.
    ~ScopedSingleton() {
        if (m_is_holder) {
            if (State expected = State::RUNNING; s_state.compare_exchange_strong(expected, State::DESTROYING)) {
                s_instance.reset();
                std::cout << "destroying" << std::endl;
            }
            s_state.store(State::EMPTY);
        }
    }

    /// Whether or not this instance is the one determining the lifetime of the static instance of `T`.
    bool is_holder() const noexcept { return m_is_holder; }

    /// @{
    /// Access operators.
    T* operator->() noexcept { return &_get(); }
    T& operator*() noexcept { return _get(); }
    const T* operator->() const noexcept { return &_get(); }
    const T& operator*() const noexcept { return _get(); }
    /// @}

private:
    /// The static instance of `T`.
    /// If no `ScopedSingleton<T>` instance is alife, this throws a SingletonError.
    static T& _get() {
        if (NOTF_LIKELY(s_state == State::RUNNING)) { return s_instance.value(); }
        NOTF_THROW(SingletonError, "No instance of ScopedSingleton<{}> exists", type_name<T>());
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Whether or not this instance is the one determining the lifetime of the static instance of `T`.
    std::atomic_bool m_is_holder = false;

    /// EMPTY -> INITIALIZING -> RUNNING -> DESTROYING -> EMPTY
    inline static std::atomic<State> s_state = State::EMPTY;

    /// The static instance of `T`.
    inline static std::optional<T> s_instance;
};

struct Foo {
    int i = 42;
};

int main() {
    std::vector<std::thread> threads(10);
    std::atomic_bool ready = false;
    std::atomic_uint error_count = 0;

    std::cout << "derbe" << std::endl;
    auto x = ScopedSingleton<Foo>();
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::thread([i, &ready, &error_count] {
            while (!ready) {}
            try {
//                ScopedSingleton<Foo>();
                int answer = (*ScopedSingleton<Foo>()).i;
            }
            catch (...) {
                ++error_count;
            }
            std::cout << i;
        });
    }
    ready = true;
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    std::cout << std::endl;
    std::cout << "error count: " << error_count << std::endl;
    return 0;
}
