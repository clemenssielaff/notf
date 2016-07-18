//
// The MIT License (MIT)
//
// Copyright (c) 2016 by Clemens Sielaff
// Copyright (c) 2013 by Konstantin (Kosta) Baumann & Autodesk Inc.
//
// Permission is hereby granted, free of charge,  to any person obtaining a copy of
// this software and  associated documentation  files  (the "Software"), to deal in
// the  Software  without  restriction,  including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software,  and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this  permission notice  shall be included in all
// copies or substantial portions of the Software.
//
// THE  SOFTWARE  IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE  LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
// IN  AN  ACTION  OF  CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

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

namespace untitled {

///
/// \brief RAII helper to make sure the call count is always reset, even in case of an exception.
///
struct CallCountGuard {
    ///
    /// \brief Value constructor.
    ///
    /// \param counter  Atomic counter to guard.
    ///
    CallCountGuard(std::atomic_uint& counter)
        : m_counter(counter)
    {
        ++m_counter;
    }

    ///
    /// \brief Destructor.
    ///
    ~CallCountGuard() { --m_counter; }

    CallCountGuard(CallCountGuard const&) = delete;
    CallCountGuard& operator=(CallCountGuard const&) = delete;
    CallCountGuard(CallCountGuard&&) = delete;
    CallCountGuard& operator=(CallCountGuard&&) = delete;

private: // fields
    ///
    /// \brief Atomic counter to guard.
    ///
    std::atomic_uint& m_counter;
};

///
/// \brief Data block shared by multiple instances of Connection.
///
template <typename... ARGUMENTS>
struct Data {
    ///
    /// \brief The number of currently active calls routed through this connection.
    ///
    std::atomic_uint running_calls{ 0 };

    ///
    /// \brief Is the connection still active?
    ///
    std::atomic_bool is_connected{ true };

    ///
    /// \brief The signal is only passed over this Connection if this function evaluates to true.
    ///
    std::function<bool(ARGUMENTS...)> test_function{ [](ARGUMENTS...) { return true; } };
};

///
/// \brief A connection between a signal and a callback.
///
template <typename... ARGUMENTS>
struct Connection {

    template <typename...>
    friend class Signal;

private: // methods for Signal
    ///
    /// \brief Value constructor.
    ///
    /// \param data Data block, potentially shared by multiple Connection objects.
    ///
    Connection(std::shared_ptr<Data<ARGUMENTS...> > data)
        : m_data(std::move(data))
    {
    }

public: // methods
    Connection(Connection const&) = default;
    Connection& operator=(Connection const&) = default;
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    ///
    /// \brief Check if the connection is (still) alive.
    ///
    /// \return True if the connection is alive.
    ///
    bool is_connected() const { return m_data && m_data->is_connected; }

    ///
    /// \brief Breaks this Connection.
    ///
    /// After calling this function, future signals will not be delivered.
    /// Any active (issued, but not handled) calls are permitted to finish.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    ///
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
    ///
    /// \brief Data, shared by multiple instances.
    ///
    std::shared_ptr<Data<ARGUMENTS...> > m_data;
};

///
/// \brief Manager class owned by instances that have methods connected to Signals.
///
/// A Slots instance tracks all Connections into an object and disconnects them when that object goes out of scope.
/// The Slots member should be placed at the end of the class definition, so it is destructed before any other.
/// This way, all data required for the last remaining calls to finish is still valid.
/// The destructor of the Slots class blocks until all calls have been handled.
///
template <typename... ARGUMENTS>
class Slots {

public: // methods
    ///
    /// \brief Default Constructor.
    ///
    Slots()
        : m_connections()
    {
    }

    ///
    /// \brief Destructor.
    ///
    /// Disconnects (blocking) all remaining connections.
    ///
    ~Slots() { disconnect_all(true); }

    Slots(Slots const&) = delete;
    Slots& operator=(Slots const&) = delete;
    Slots(Slots&&) = delete;
    Slots& operator=(Slots&&) = delete;

    ///
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
    ///
    template <typename SIGNAL, typename... ARGS>
    void connect(SIGNAL& signal, ARGS&&... args)
    {
        // calls one of the two Signal.connect() overloads, depending whether you specify a single function object as
        // arguments or a pointer/method-address pair
        Connection<ARGUMENTS...> connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.emplace_back(std::move(connection));
    }

    ///
    /// \brief Disconnects all tracked Connections.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    ///
    void disconnect_all(bool block = false)
    {
        // first disconnect all connections without waiting
        for (Connection<ARGUMENTS...>& connection : m_connections) {
            connection.disconnect(false);
        }

        // ... then wait for them (if requested)
        if (block) {
            for (Connection<ARGUMENTS...>& connection : m_connections) {
                connection.disconnect(true);
            }
        }
        m_connections.clear();
    }

private: // fields
    ///
    /// \brief All managed Connections.
    ///
    std::vector<Connection<ARGUMENTS...> > m_connections;
};

///
/// \brief An object capable of firing (emitting) signals to connected callbacks.
///
template <typename... ARGUMENTS>
class Signal {

    template <typename...>
    friend class Slots;

private: // struct
    ///
    /// \brief Connection and target function pair.
    ///
    struct Callback {
        Callback(Connection<ARGUMENTS...> connection, std::function<void(ARGUMENTS...)> function)
            : connection(std::move(connection))
            , function(std::move(function))
        {
        }

        ///
        /// \brief Connection through which the Callback is performed.
        ///
        Connection<ARGUMENTS...> connection;

        ///
        /// \brief Callback function.
        ///
        std::function<void(ARGUMENTS...)> function;
    };

public: // methods
    ///
    /// \brief Default constructor.
    ///
    Signal()
        : m_mutex()
        , m_targets()
    {
    }

    ///
    /// \brief Destructor.
    ///
    /// Blocks until all Connections are disconnected.
    ///
    ~Signal() { disconnect_all(true); }

    Signal(Signal const&) = delete;
    Signal& operator=(Signal const&) = delete;

    ///
    /// \brief Move Constructor.
    ///
    /// \param other
    ///
    Signal(Signal&& other) noexcept
    {
        this = other;
    }

    ///
    /// \brief RValue assignment Operator.
    ///
    /// \param other
    ///
    /// \return This instance.
    ///
    Signal& operator=(Signal&& other) noexcept
    {
        // use std::lock(...) in combination with std::defer_lock to acquire two locks
        // without worrying about potential deadlocks (see: http://en.cppreference.com/w/cpp/thread/lock)
        std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
        std::unique_lock<std::mutex> lock2(other.m_mutex, std::defer_lock);
        std::lock(lock1, lock2);

        m_targets = std::move(other.m_targets);
        return *this;
    }

    ///
    /// \brief Connects a new callback target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new callback is connected.
    ///
    /// \param target   Target callback.
    ///
    /// \return The created Connection.
    ///
    Connection<ARGUMENTS...> connect(std::function<void(ARGUMENTS...)> target)
    {
        assert(target);

        // create a new targets vector (will be filled in with
        // the existing and still active targets within the lock below)
        auto new_targets = std::make_shared<std::vector<Callback> >();

        // lock the mutex for writing
        std::lock_guard<std::mutex> lock(m_mutex);
        UNUSED(lock);

        // copy existing, connected  targets
        if (auto current_targets = m_targets) {
            new_targets->reserve(current_targets->size() + 1);
            for (const auto& target : *current_targets) {
                if (target.connection.is_connected()) {
                    new_targets->push_back(target);
                }
            }
        }

        // add the new connection to the new vector
        auto connection = Connection<ARGUMENTS...>(std::make_shared<Data<ARGUMENTS...> >());
        new_targets->emplace_back(connection, std::move(target));

        // replace the pointer to the targets (in a thread safe manner)
        std::swap(m_targets, new_targets);

        return connection;
    }

    ///
    /// \brief Disconnect all Connections from this Signal.
    ///
    /// \param block    If set, this function blocks until all active calls have finished.
    ///                 Otherwise it returns immediately.
    ///
    void disconnect_all(bool block = false)
    {
        auto leftover_targets = decltype(m_targets)(nullptr);

        // clean out the target pointers so no other thread will fire this signal anymore
        // (already running fired calls might still reference the targets)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            UNUSED(lock);
            std::swap(m_targets, leftover_targets); // replace m_targets pointer with a nullptr
        }

        // disconnect all targets
        if (leftover_targets) {
            for (auto&& target : *leftover_targets) {
                target.connection.disconnect(block);
            }
        }
    }

    ///
    /// \brief Fires (emits) the signal.
    ///
    /// Arguments to this function are passed to the connected callbacks and must match the signature with which the
    /// Signal instance was defined.
    ///
    void fire(ARGUMENTS&... args) const
    {
        auto targets = m_targets;
        if (!targets) {
            return;
        }
        // no lock required here as m_targets is never modified, only replaced, and we iterate over our own copy here
        for (auto& target : *targets) {
            auto& connection = target.connection;
            if (!connection.is_connected() || !connection.m_data->test_function(args...)) {
                continue;
            }
            CallCountGuard callCountGuard(connection.m_data->running_calls);
            UNUSED(callCountGuard);
            target.function(args...);
        }
    }

private: // methods for Connections
    ///
    /// \brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
    template <typename OBJ>
    Connection<ARGUMENTS...> connect(OBJ* obj, void (OBJ::*method)(ARGUMENTS... args))
    {
        assert(obj);
        assert(method);
        return connect([=](ARGUMENTS... args) { (obj->*method)(args...); });
    }

    ///
    /// \brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
//    template <typename OBJ>
//    Connection<ARGUMENTS...> connect(OBJ* obj, std::function<bool(ARGUMENTS...)> test_func, void (OBJ::*method)(ARGUMENTS... args))
//    {
//        assert(obj);
//        assert(method);
//        auto blub = connect([=](ARGUMENTS... args) { (obj->*method)(args...); });
//        blub.m_data->test_function = test_func;
//        return blub;
//    }
    // TODO: make implement this into the connect() function

private: // fields
    ///
    /// \brief Mutex required to write to the 'm_targets' field.
    ///
    mutable std::mutex m_mutex;

    ///
    /// \brief All targets of this Signal.
    ///
    /// Is wrapped in a shared_ptr replace the contents in a thread-safe manner.
    ///
    std::shared_ptr<std::vector<Callback> > m_targets;
};

} // namespace untitled
