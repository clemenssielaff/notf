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
#include <memory>
#include <vector>

namespace untitled {

///
/// \brief A connection between a signal and a callback.
///
struct Connection {

    template <typename...>
    friend class Signal;

private: //struct
    ///
    /// \brief Data block shared by multiple instances of Connection.
    ///
    struct Data {
        ///
        /// \brief Is the connection still active?
        ///
        bool is_connected{ true };
    };

private: // methods for Signal
    ///
    /// \brief Value constructor.
    ///
    /// \param data Data block, potentially shared by multiple Connection objects.
    ///
    Connection(std::shared_ptr<Data> data)
        : m_data(std::move(data))
    {
    }

    static Connection make_connection()
    {
        return Connection(std::make_shared<Data>());
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
    void disconnect()
    {
        if (!m_data) {
            return;
        }
        m_data->is_connected = false;
    }

private: // fields
    ///
    /// \brief Data, shared by multiple instances.
    ///
    std::shared_ptr<Data> m_data;
};

///
/// \brief Manager class owned by instances that have methods connected to Signals.
///
/// A Slots instance tracks all Connections into an object and disconnects them when that object goes out of scope.
/// The Slots member should be placed at the end of the class definition, so it is destructed before any other.
/// This way, all data required for the last remaining calls to finish is still valid.
/// The destructor of the Slots class blocks until all calls have been handled.
///
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
    ~Slots() { disconnect_all(); }

    Slots(Slots const&) = delete;
    Slots& operator=(Slots const&) = delete;
    Slots(Slots&&) = delete;
    Slots& operator=(Slots&&) = delete;

    ///
    /// \brief Creates and tracks a new Connection between the Signal and callback function.
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
        Connection connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.emplace_back(std::move(connection));
    }

    ///
    /// \brief Disconnects all tracked Connections.
    ///
    void disconnect_all()
    {
        for (Connection& connection : m_connections) {
            connection.disconnect();
        }
        m_connections.clear();
    }

private: // fields
    ///
    /// \brief All managed Connections.
    ///
    std::vector<Connection> m_connections;
};

///
/// \brief An object capable of firing (emitting) signals to connected callbacks.
///
template <typename... ARGUMENTS>
class Signal {

    friend class Slots;

private: // struct
    ///
    /// \brief Connection and target function pair.
    ///
    struct Callback {
        Callback(Connection connection, std::function<void(ARGUMENTS...)> function,
            std::function<bool(ARGUMENTS...)> test_function = [](ARGUMENTS...) { return true; })
            : connection(std::move(connection))
            , function(std::move(function))
            , test_function(std::move(test_function))
        {
        }

        ///
        /// \brief Connection through which the Callback is performed.
        ///
        Connection connection;

        ///
        /// \brief Callback function.
        ///
        std::function<void(ARGUMENTS...)> function;

        ///
        /// \brief The signal is only passed over this Connection if this function evaluates to true.
        ///
        std::function<bool(ARGUMENTS...)> test_function;
    };

public: // methods
    ///
    /// \brief Default constructor.
    ///
    Signal()
        : m_callbacks()
    {
    }

    ///
    /// \brief Destructor.
    ///
    ~Signal() { disconnect_all(); }

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
        m_callbacks = std::move(other.m_callbacks);
        return *this;
    }

    ///
    /// \brief Connects a new callback target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new callback is connected.
    ///
    /// \param callback     Target callback.
    /// \param test_func    (optional) Test function. Callback is always called when empty.
    ///
    /// \return The created Connection.
    ///
    Connection connect(std::function<void(ARGUMENTS...)> callback,
        std::function<bool(ARGUMENTS...)> test_func = {})
    {
        assert(callback);

        // create a new callback vector (will be filled in with
        // the existing and still active callbacks within the lock below)
        auto new_callbacks = std::vector<Callback>();

        // copy existing, connected  targets
        new_callbacks.reserve(m_callbacks.size() + 1);
        for (const auto& target : m_callbacks) {
            if (target.connection.is_connected()) {
                new_callbacks.push_back(target);
            }
        }

        // add the new connection to the new vector
        Connection connection = Connection::make_connection();
        if (test_func) {
            new_callbacks.emplace_back(connection, std::move(callback), std::move(test_func));
        } else {
            new_callbacks.emplace_back(connection, std::move(callback));
        }

        // replace the stored callbacks
        std::swap(m_callbacks, new_callbacks);

        return connection;
    }

    ///
    /// \brief Disconnect all Connections from this Signal.
    ///
    void disconnect_all()
    {
        // disconnect all callbacks
        for (auto& callback : m_callbacks) {
            callback.connection.disconnect();
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
        for (auto& callback : m_callbacks) {
            if (!callback.connection.is_connected() || !callback.test_function(args...)) {
                continue;
            }
            callback.function(args...);
        }
    }

private: // methods for Connections
    ///
    /// \brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
    /// \param obj          Instance providing the callback.
    /// \param method       Address of the method.
    /// \param test_func    (optional) Test function. Callback is always called when empty.
    ///
    template <typename OBJ>
    Connection connect(OBJ* obj, void (OBJ::*method)(ARGUMENTS... args), std::function<bool(ARGUMENTS...)> test_func = {})
    {
        assert(obj);
        assert(method);
        return connect([=](ARGUMENTS... args) { (obj->*method)(args...); }, std::forward<std::function<bool(ARGUMENTS...)> >(test_func));
    }

private: // fields
    ///
    /// \brief All target callbacks of this Signal.
    ///
    std::vector<Callback> m_callbacks;
};

} // namespace untitled
