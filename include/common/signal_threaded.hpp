#pragma once

#include <assert.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifndef UNUSED
#define UNUSED(x) (void)(x);
#endif

namespace signal {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief A connection between a signal and a callback.
struct ThreadedConnection {

    template <typename...>
    friend class ThreadedSignal;

private: //struct
    /// \brief Data block shared by two ThreadedConnection instances.
    struct Data {
        /// \brief The number of currently active calls routed through this connection.
        std::atomic_uint running_calls{ 0 };

        /// \brief Is the connection still active?
        std::atomic_bool is_connected{ true };
    };

private: // methods for ThreadedSignal
    /// \brief Value constructor.
    ///
    /// \param data Shared ThreadedConnection data block.
    ThreadedConnection(std::shared_ptr<Data> data)
        : m_data(std::move(data))
    {
    }

    /// \brief Creates a new, default constructed Connections object.
    static ThreadedConnection make_connection()
    {
        return ThreadedConnection(std::make_shared<Data>());
    }

public: // methods
    ThreadedConnection(ThreadedConnection const&) = default;
    ThreadedConnection& operator=(ThreadedConnection const&) = default;
    ThreadedConnection(ThreadedConnection&&) = default;
    ThreadedConnection& operator=(ThreadedConnection&&) = default;

    /// \brief Check if the connection is alive.
    ///
    /// \return True if the connection is alive.
    bool is_connected() const { return m_data && m_data->is_connected; }

    /// \brief Breaks this Connection.
    ///
    /// After calling this function, future signals will not be delivered.
    /// Any active (issued, but not handled) calls are permitted to finish.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    void disconnect(bool block = false)
    {
        if (!m_data) {
            return;
        }
        m_data->is_connected.store(false, std::memory_order_release);

        if (block) {
            while (m_data->running_calls) {
                std::this_thread::yield();
            }
        }
        return;
    }

private: // fields
    /// \brief Data block shared by two ThreadedConnection instances.
    std::shared_ptr<Data> m_data;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Manager class owned by instances that have methods connected to Signals.
///
/// A Callback instance tracks all Connections representing Signal Targets to a member function of an object,
/// and disconnects them when that object goes out of scope.
/// The Callback member should be placed at the end of the class definition, so it is destructed before any other.
/// This way, all data required for the last remaining calls to finish is still valid.
/// The destructor of the Slots class blocks until all calls have been handled.
/// If used within a class hierarchy, the most specialized class has the responsibility to disconnect all of its base
/// class' Signals before destroying any other members.
class ThreadedCallback {

public: // methods
    /// \brief Default Constructor.
    ThreadedCallback()
        : m_connections()
    {
    }

    /// \brief Destructor.
    ///
    /// Disconnects (blocking) all remaining connections.
    ~ThreadedCallback() { disconnect_all(true); }

    ThreadedCallback(ThreadedCallback const&) = delete;
    ThreadedCallback& operator=(ThreadedCallback const&) = delete;
    ThreadedCallback(ThreadedCallback&&) = delete;
    ThreadedCallback& operator=(ThreadedCallback&&) = delete;

    /// \brief Creates and tracks a new Connection between the Signal and target function.
    ///
    /// All arguments after the initial 'Signal' are passed to Signal::connect().
    ///
    /// Connect a method of 'receiver' like this:
    ///     receiver->slots.connect(sender.signal, receiver, &Reciver::callback)
    ///
    /// Can also be used to connect to a lambda expression like so:
    ///     receiver->slots.connect(sender.signal, [](){ /* lambda here */ });
    ///
    /// \brief signal   The Signal to connect to.
    /// \brief args     Arguments passed to Signal::connect().
    template <typename SIGNAL, typename... ARGS>
    void connect(SIGNAL& signal, ARGS&&... args)
    {
        // calls one of the two Signal.connect() overloads, depending whether you specify a single function object as
        // arguments or a pointer/method-address pair
        ThreadedConnection connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.emplace_back(std::move(connection));
    }

    /// \brief Disconnects all tracked Connections.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    void disconnect_all(bool block = false)
    {
        // first disconnect all connections without waiting
        for (ThreadedConnection& connection : m_connections) {
            connection.disconnect(false);
        }

        // ... then wait for them (if requested)
        if (block) {
            for (ThreadedConnection& connection : m_connections) {
                connection.disconnect(true);
            }
        }
        m_connections.clear();
    }

private: // fields
    /// \brief All managed Connections.
    std::vector<ThreadedConnection> m_connections;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief An object capable of firing (emitting) signals to connected targets.
template <typename... ARGUMENTS>
class ThreadedSignal {

    friend class ThreadedCallback;

private: // struct
    /// \brief Connection and target function pair.
    struct Target {
        Target(ThreadedConnection connection, std::function<void(ARGUMENTS...)> function,
            std::function<bool(ARGUMENTS...)> test_function = [](ARGUMENTS...) { return true; })
            : connection(std::move(connection))
            , function(std::move(function))
            , test_function(std::move(test_function))
        {
        }

        /// \brief Connection through which the Callback is performed.
        ThreadedConnection connection;

        /// \brief Callback function.
        std::function<void(ARGUMENTS...)> function;

        /// \brief The signal is only passed over this Connection if this function evaluates to true.
        std::function<bool(ARGUMENTS...)> test_function;
    };

    /// \brief RAII helper to make sure the call count is always reset, even in case of an exception.
    struct CallCountGuard {
        /// \brief Value constructor.
        ///
        /// \param counter  Atomic counter to guard.
        CallCountGuard(std::atomic_uint& counter)
            : m_counter(counter)
        {
            ++m_counter;
        }

        /// \brief Destructor.
        ~CallCountGuard() { --m_counter; }

        CallCountGuard(CallCountGuard const&) = delete;
        CallCountGuard& operator=(CallCountGuard const&) = delete;
        CallCountGuard(CallCountGuard&&) = delete;
        CallCountGuard& operator=(CallCountGuard&&) = delete;

    private: // fields
        /// \brief Atomic counter to guard.
        std::atomic_uint& m_counter;
    };

public: // methods
    /// \brief Default constructor.
    ThreadedSignal()
        : m_mutex()
        , m_targets()
    {
    }

    /// \brief Destructor.
    ///
    /// Blocks until all Connections are disconnected.
    ~ThreadedSignal()
    {
        disconnect_all(true);
    }

    ThreadedSignal(ThreadedSignal const&) = delete;
    ThreadedSignal& operator=(ThreadedSignal const&) = delete;

    /// \brief Move Constructor.
    ///
    /// \param other
    ThreadedSignal(ThreadedSignal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

    /// \brief RValue assignment Operator.
    ///
    /// \param other
    ///
    /// \return This instance.
    ThreadedSignal& operator=(ThreadedSignal&& other) noexcept
    {
        // use std::lock(...) in combination with std::defer_lock to acquire two locks
        // without worrying about potential deadlocks (see: http://en.cppreference.com/w/cpp/thread/lock)
        std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
        std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
        std::lock(lock1, lock2);

        m_targets = std::move(other.m_targets);
        return *this;
    }

    /// \brief Connects a new target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new target is connected.
    ///
    /// \param function     Target function.
    /// \param test_func    (optional) Test function. Target is always called when empty.
    ///
    /// \return The created Connection.
    ThreadedConnection connect(std::function<void(ARGUMENTS...)> function,
        std::function<bool(ARGUMENTS...)> test_func = {})
    {
        assert(function);

        // create a new target vector (will be filled in with
        // the existing and still active targets within the lock below)
        auto new_targets = std::make_shared<std::vector<Target> >();

        // lock the mutex for writing
        std::lock_guard<std::mutex> lock(m_mutex);
        UNUSED(lock);

        // copy existing, connected targets
        if (auto current_targets = m_targets) {
            new_targets->reserve(current_targets->size() + 1);
            for (const auto& target : *current_targets) {
                if (target.connection.is_connected()) {
                    new_targets->push_back(target);
                }
            }
        }

        // add the new connection to the new vector
        ThreadedConnection connection = ThreadedConnection::make_connection();
        if (test_func) {
            new_targets->emplace_back(connection, std::move(function), std::move(test_func));
        } else {
            new_targets->emplace_back(connection, std::move(function));
        }

        // replace the stored targets
        std::swap(m_targets, new_targets);

        return connection;
    }

    /// \brief Disconnect all Connections from this Signal.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    void disconnect_all(bool block = false)
    {
        auto leftover_targets = decltype(m_targets)(nullptr);

        // clean out the callback pointers so no other thread will fire this signal anymore
        // (already running fired calls might still reference the callbacks)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            UNUSED(lock);
            std::swap(m_targets, leftover_targets); // replace m_callbacks pointer with a nullptr
        }

        // disconnect all callbacks
        if (leftover_targets) {
            for (auto& target : *leftover_targets) {
                target.connection.disconnect(block);
            }
        }
    }

    /// \brief Fires (emits) the signal.
    ///
    /// Arguments to this function are passed to the connected callbacks and must match the signature with which the
    /// Signal instance was defined.
    void fire(ARGUMENTS&... args) const
    {
        auto targets = m_targets;
        if (!targets) {
            return;
        }
        // no lock required here as m_callbacks is never modified, only replaced, and we iterate over our own copy here
        for (auto& callback : *targets) {
            if (!callback.connection.is_connected() || !callback.test_function(args...)) {
                continue;
            }
            CallCountGuard callCountGuard(callback.connection.m_data->running_calls);
            UNUSED(callCountGuard);
            callback.function(args...);
        }
    }

private: // methods for Connections
    /// \brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
    /// \param obj          Instance providing the callback.
    /// \param method       Address of the method.
    /// \param test_func    (optional) Test function. Callback is always called when empty.
    template <typename OBJ>
    ThreadedConnection connect(OBJ* obj, void (OBJ::*method)(ARGUMENTS... args), std::function<bool(ARGUMENTS...)> test_func = {})
    {
        assert(obj);
        assert(method);
        return connect([=](ARGUMENTS... args) { (obj->*method)(args...); }, std::forward<std::function<bool(ARGUMENTS...)> >(test_func));
    }

private: // fields
    /// \brief Mutex required to write to the 'm_targets' field.
    mutable std::mutex m_mutex;

    /// \brief All targets of this Signal.
    ///
    /// Is wrapped in a shared_ptr replace the contents in a thread-safe manner.
    std::shared_ptr<std::vector<Target> > m_targets;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Full specialization for Signals that require no arguments.
template <>
class ThreadedSignal<> {

    friend class ThreadedCallback;

private: // struct
    /// \brief Connection and target function pair.
    struct Target {
        Target(ThreadedConnection connection, std::function<void()> function)
            : connection(std::move(connection))
            , function(std::move(function))
        {
        }

        /// \brief Connection through which the Callback is performed.
        ThreadedConnection connection;

        /// \brief Callback function.
        std::function<void()> function;
    };

    /// \brief RAII helper to make sure the call count is always reset, even in case of an exception.
    struct CallCountGuard {
        /// \brief Value constructor.
        ///
        /// \param counter  Atomic counter to guard.
        CallCountGuard(std::atomic_uint& counter)
            : m_counter(counter)
        {
            ++m_counter;
        }

        /// \brief Destructor.
        ~CallCountGuard() { --m_counter; }

        CallCountGuard(CallCountGuard const&) = delete;
        CallCountGuard& operator=(CallCountGuard const&) = delete;
        CallCountGuard(CallCountGuard&&) = delete;
        CallCountGuard& operator=(CallCountGuard&&) = delete;

    private: // fields
        /// \brief Atomic counter to guard.
        std::atomic_uint& m_counter;
    };

public: // methods
    /// \brief Default constructor.
    ThreadedSignal()
        : m_mutex()
        , m_targets()
    {
    }

    /// \brief Destructor.
    ///
    /// Blocks until all Connections are disconnected.
    ~ThreadedSignal()
    {
        disconnect_all(true);
    }

    ThreadedSignal(ThreadedSignal const&) = delete;
    ThreadedSignal& operator=(ThreadedSignal const&) = delete;

    /// \brief Move Constructor.
    ///
    /// \param other
    ThreadedSignal(ThreadedSignal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

    /// \brief RValue assignment Operator.
    ///
    /// \param other
    ///
    /// \return This instance.
    ThreadedSignal& operator=(ThreadedSignal&& other) noexcept
    {
        // use std::lock(...) in combination with std::defer_lock to acquire two locks
        // without worrying about potential deadlocks (see: http://en.cppreference.com/w/cpp/thread/lock)
        std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
        std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
        std::lock(lock1, lock2);

        m_targets = std::move(other.m_targets);
        return *this;
    }

    /// \brief Connects a new target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new target is connected.
    ///
    /// \param function     Target function.
    /// \param test_func    (optional) Test function. Target is always called when empty.
    ///
    /// \return The created Connection.
    ThreadedConnection connect(std::function<void()> function)
    {
        assert(function);

        // create a new target vector (will be filled in with
        // the existing and still active targets within the lock below)
        auto new_targets = std::make_shared<std::vector<Target> >();

        // lock the mutex for writing
        std::lock_guard<std::mutex> lock(m_mutex);
        UNUSED(lock);

        // copy existing, connected targets
        if (auto current_targets = m_targets) {
            new_targets->reserve(current_targets->size() + 1);
            for (const auto& target : *current_targets) {
                if (target.connection.is_connected()) {
                    new_targets->push_back(target);
                }
            }
        }

        // add the new connection to the new vector
        ThreadedConnection connection = ThreadedConnection::make_connection();
        new_targets->emplace_back(connection, std::move(function));

        // replace the stored targets
        std::swap(m_targets, new_targets);

        return connection;
    }

    /// \brief Disconnect all Connections from this Signal.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    void disconnect_all(bool block = false)
    {
        auto leftover_targets = decltype(m_targets)(nullptr);

        // clean out the callback pointers so no other thread will fire this signal anymore
        // (already running fired calls might still reference the callbacks)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            UNUSED(lock);
            std::swap(m_targets, leftover_targets); // replace m_callbacks pointer with a nullptr
        }

        // disconnect all callbacks
        if (leftover_targets) {
            for (auto& target : *leftover_targets) {
                target.connection.disconnect(block);
            }
        }
    }

    /// \brief Fires (emits) the signal.
    ///
    /// Arguments to this function are passed to the connected callbacks and must match the signature with which the
    /// Signal instance was defined.
    void fire() const
    {
        auto targets = m_targets;
        if (!targets) {
            return;
        }
        // no lock required here as m_callbacks is never modified, only replaced, and we iterate over our own copy here
        for (auto& callback : *targets) {
            if (!callback.connection.is_connected()) {
                continue;
            }
            CallCountGuard callCountGuard(callback.connection.m_data->running_calls);
            UNUSED(callCountGuard);
            callback.function();
        }
    }

private: // methods for Connections
    /// \brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
    /// \param obj          Instance providing the callback.
    /// \param method       Address of the method.
    /// \param test_func    (optional) Test function. Callback is always called when empty.
    template <typename OBJ>
    ThreadedConnection connect(OBJ* obj, void (OBJ::*method)())
    {
        assert(obj);
        assert(method);
        return connect([=]() { (obj->*method)(); });
    }

private: // fields
    /// \brief Mutex required to write to the 'm_targets' field.
    mutable std::mutex m_mutex;

    /// \brief All targets of this Signal.
    ///
    /// Is wrapped in a shared_ptr replace the contents in a thread-safe manner.
    std::shared_ptr<std::vector<Target> > m_targets;
};

} // namespace signal
