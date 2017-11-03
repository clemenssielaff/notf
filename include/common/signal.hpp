/*
 *
 * Gotchas
 * =======
 * Connecting Signals to member functions is probably the trickiest part of signal management.
 * It's easy but also easy to get wrong when you don't think about what you are doing.
 * Imagine the following case.
 * We have Signal 'S' and some object instance 'I' with member function 'm'.
 * In the constructor of 'I' we want to connect 'S' to 'm' of I.
 * The first (my first) instinct would be to write:
 *
 *     S.connnect([this]() -> void {m();});
 *
 * and everything would be fine.
 * Until 'I' goes out of scope and 'S' is left with a lambda pointing to the location of deleted memory.
 * What you instead need to first make sure, that 'I' derives from 'receive_signals' and then use this is call:
 *
 *     I.connect_signal(S, m);
 *
 * This has the same effect, but in addition of allowing some more magic (connecting Signal<ARGS...> to methods with
 * no parameters, for example) it is also save for the case outlined above.
 * The newly created Connection is stored internally in 'I', and when 'I' goes out of scope, 'S' will automatically be
 * disconnected.
 *
 * There isn't really much else to say - we want the ability of connecting Signals to free functions, so we cannot
 * get rid of the connection to a lambda.
 * At the same time, lambdas require the captured objects to outlive the lambda - something that isn't guaranteed when
 * the lambda itself is stored in the Signal which may live anywhere in the code.
 */

#pragma once

#include <algorithm>
#include <assert.h>
#include <exception>
#include <functional>
#include <memory>
#include <vector>

#include "common/meta.hpp"

namespace notf {

/**********************************************************************************************************************/

namespace detail {

/** Guard making sure that a Signal's `is_firing` flag is always unset when the Signal has finished firing. */
struct FlagGuard {
    FlagGuard(bool& flag) : m_flag(flag) { m_flag = true; }

    ~FlagGuard() { m_flag = false; }

    bool& m_flag;
};

} // namespace detail

/**********************************************************************************************************************/

/** Internal data for a Connection between a Signal and a Callback. */
struct Connection {

    template<typename...>
    friend class Signal;

private: // struct
    /** Data block shared by all instances of the same logical Connection.
     * That means, at least the Signal owns a Connection object, as does the `receive_signals` mixins or even the user.
     */
    struct Data {
        /** Is the connection still active? */
        bool is_connected = true;

        /** Is the connection currently enabled? */
        bool is_enabled = true;
    };

public: // methods
    /** Creates a new, default constructed ConnectionData object. */
    static Connection create() { return Connection(std::make_shared<Data>()); }

public: // methods
    /** Default (empty) Connection. */
    Connection() : m_data() {}

    /** Check if the connection is alive. */
    bool is_connected() const { return m_data && m_data->is_connected; }

    /** Checks if the Connection is currently enabled. */
    bool is_enabled() const { return m_data && m_data->is_enabled; }

    /** Checks if two Connection objects identify the same logical connection. */
    bool operator==(const Connection& other) const { return m_data == other.m_data; }

    /** Enables this Connection (does nothing when disconnected). */
    void enable() const
    {
        if (m_data) {
            m_data->is_enabled = true;
        }
    }

    /** Disables this Connection (does nothing when disconnected). */
    void disable() const
    {
        if (m_data) {
            m_data->is_enabled = false;
        }
    }

    /** Breaks this Connection.
     * After calling this function, future signals will not be delivered.
     */
    void disconnect()
    {
        if (!m_data) {
            return;
        }
        m_data->is_connected = false;
    }

private: // methods
    /** @param data Shared ConnectionData block. */
    Connection(std::shared_ptr<Data> data) : m_data(std::move(data)) {}

private: // fields
    /** Data block shared by two Connection instances. */
    std::shared_ptr<Data> m_data;
};

/**********************************************************************************************************************/

/** An object capable of firing (emitting) signals to connected targets. */
template<typename... SIGNATURE>
class Signal {

public: // type
    using Signature = std::tuple<SIGNATURE...>;

private: // struct
    /** Connection and target function pair. */
    struct Target {
        Target(Connection connection, std::function<void(SIGNATURE...)> function,
               std::function<bool(SIGNATURE...)> test_function)
            : connection(std::move(connection)), function(std::move(function)), test_function(std::move(test_function))
        {
        }

        /** Connection through which the Callback is performed. */
        Connection connection;

        /** Callback function. */
        std::function<void(SIGNATURE...)> function;

        /** The signal is only passed over this Connection if this function evaluates to true (or is empty). */
        std::function<bool(SIGNATURE...)> test_function;
    };

public: // methods
    DISALLOW_COPY_AND_ASSIGN(Signal)

    /** Constructor. */
    Signal() = default;

    /** Destructor. */
    ~Signal() { disconnect(); }

    /** Move Constructor. */
    Signal(Signal&& other) noexcept : m_targets(std::move(other.m_targets)) {}

    /** RValue assignment Operator. */
    Signal& operator=(Signal&& other) noexcept
    {
        m_targets = std::move(other.m_targets);
        return *this;
    }

    /** Connects a new target to this Signal.
     * Existing but disconnected Connections are removed before the new target is connected.
     * @param function     Target function.
     * @param test_func    (optional) Test function. When null, the Target is always called.
     * @return             The created Connection.
     */
    Connection connect(std::function<void(SIGNATURE...)> function, std::function<bool(SIGNATURE...)> test_func = {})
    {
        assert(function);

        // remove disconnected targets
        std::remove_if(m_targets.begin(), m_targets.end(),
                       [](const Target& target) -> bool { return !target.connection.is_connected(); });

        // add the new connection
        Connection connection = Connection::create();
        m_targets.emplace_back(connection, std::move(function), std::move(test_func));

        return connection;
    }

    /** Checks if a particular Connection is connected to this Signal. */
    bool has_connection(const Connection& connection) const
    {
        for (const Target& target : m_targets) {
            if (target.connection == connection) {
                return true;
            }
        }
        return false;
    }

    /** Returns a vector of all (connected) Connections.*/
    std::vector<Connection> get_connections() const
    {
        std::vector<Connection> result;
        result.reserve(m_targets.size());
        for (const Target& target : m_targets) {
            if (target.connection.is_connected()) {
                result.push_back(target.connection);
            }
        }
        return result;
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (auto& target : m_targets) {
            target.connection.enable();
        }
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (Target& target : m_targets) {
            target.connection.disable();
        }
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
        m_targets.clear();
    }

    /** Fires (emits) the signal.
     * Arguments to this function are passed to the connected callbacks and must match the signature with which the
     * Signal instance was defined.
     * Build-in type casting works as expected (float -> int etc.).
     * @throw   std::runtime_error if the Signal is part of a cyclic connection.
     */
    template<typename... ARGUMENTS>
    void operator()(ARGUMENTS&&... args) const
    {
        if (m_is_firing) {
            throw std::runtime_error("Cyclic connection detected!");
        }
        detail::FlagGuard _(m_is_firing);

        for (auto& target : m_targets) {
            if (true && target.connection.is_connected() && target.connection.is_enabled()
                && (!target.test_function || target.test_function(args...))) {
                target.function(args...);
            }
        }
    }

private: // fields
    /** All targets of this Signal. */
    std::vector<Target> m_targets;

    /** Flag to identify circular connections of Signals. */
    mutable bool m_is_firing = false;
};

/**********************************************************************************************************************/

/** Full specialization for Signals that require no arguments.
 * Since they don't have a test-function, emitting Signals without arguments is ~1.65x as fast than those without.
 */
template<>
class Signal<> {

public: // type
    using Signature = std::tuple<>;

private: // struct
    /** Connection and target function pair. */
    struct Target {
        Target(Connection connection, std::function<void()> function)
            : connection(std::move(connection)), function(std::move(function))
        {
        }

        /** Connection through which the Callback is performed. */
        Connection connection;

        /** Callback function. */
        std::function<void()> function;
    };

public: // methods
    DISALLOW_COPY_AND_ASSIGN(Signal)

    Signal() = default;

    ~Signal() { disconnect(); }

    /** Move Constructor. */
    Signal(Signal&& other) noexcept : m_targets(std::move(other.m_targets)) {}

    /** RValue assignment Operator. */
    Signal& operator=(Signal&& other) noexcept
    {
        m_targets = std::move(other.m_targets);
        return *this;
    }

    /** Connects a new target to this Signal.
     * Existing but disconnected Connections are removed before the new target is connected.
     * @param function     Target function.
     * @return             The created Connection.
     */
    Connection connect(std::function<void()> function)
    {
        assert(function);

        // remove disconnected targets
        std::remove_if(m_targets.begin(), m_targets.end(),
                       [](const Target& target) -> bool { return !target.connection.is_connected(); });

        // create a new Connection
        Connection connection = Connection::create();
        m_targets.emplace_back(connection, std::move(function));

        return connection;
    }

    /** Checks if a particular Connection is connected to this Signal. */
    bool has_connection(const Connection& connection) const
    {
        for (const Target& target : m_targets) {
            if (target.connection == connection) {
                return true;
            }
        }
        return false;
    }

    /** Returns a vector of all (connected) Connections.*/
    std::vector<Connection> get_connections() const
    {
        std::vector<Connection> result;
        result.reserve(m_targets.size());
        for (const Target& target : m_targets) {
            if (target.connection.is_connected()) {
                result.push_back(target.connection);
            }
        }
        return result;
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (auto& target : m_targets) {
            target.connection.enable();
        }
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (auto& target : m_targets) {
            target.connection.disable();
        }
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
        m_targets.clear();
    }

    /** Fires (emits) the signal.
     * @throw   std::runtime_error if the Signal is part of a cyclic connection.
     */
    void operator()() const
    {
        if (m_is_firing) {
            throw std::runtime_error("Cyclic connection detected!");
        }
        detail::FlagGuard _(m_is_firing);

        for (auto& target : m_targets) {
            if (true && target.connection.is_connected() && target.connection.is_enabled()) {
                target.function();
            }
        }
    }

private: // fields
    /** All targets of this Signal. */
    std::vector<Target> m_targets;

    /** Flag to identify circular connections of Signals. */
    mutable bool m_is_firing = false;
};

/**********************************************************************************************************************/

/** Mixin class to be able to receive Signals. */
class receive_signals {

public: // methods
    DISALLOW_COPY_AND_ASSIGN(receive_signals)

    /** Constructor. */
    receive_signals() = default;

    /** Destructor. */
    virtual ~receive_signals();

    // 1st overload

    /** Creates a connection managed by this object, connecting the given signal to a function object. */
    template<typename SIGNAL, typename... ARGS>
    Connection connect_signal(SIGNAL& signal, ARGS&&... args)
    {
        return _connect_signal(signal, std::forward<ARGS>(args)...);
    }

    // 2nd overload

    /** Creates a Connection connecting the given Signal to a member function of this object. */
    template<typename SIGNAL, typename RECEIVER, typename... SIGNATURE, typename... ARGS,
             typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value == sizeof...(SIGNATURE))>>
    Connection connect_signal(SIGNAL& signal, void (RECEIVER::*method)(SIGNATURE...), ARGS&&... args)
    {
        return _connect_signal(signal,
                               [this, method](SIGNATURE... args) { (static_cast<RECEIVER*>(this)->*method)(args...); },
                               std::forward<ARGS>(args)...);
    }

    /** Creates a Connection connecting the given Signal to a CONST member function of this object. */
    template<typename SIGNAL, typename RECEIVER, typename... SIGNATURE, typename... ARGS,
             typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value == sizeof...(SIGNATURE))>>
    Connection connect_signal(SIGNAL& signal, void (RECEIVER::*method)(SIGNATURE...) const, ARGS&&... args)
    {
        return _connect_signal(signal,
                               [this, method](SIGNATURE... args) { (static_cast<RECEIVER*>(this)->*method)(args...); },
                               std::forward<ARGS>(args)...);
    }

    // 3rd overload

    /** Overload to connect any Signal to a member function without arguments.
     * This way, you can connect a `Signal<int>` to a function `foo()` without having to manually wrap the function
     * call in a lambda that discards the additional arguments.
     */
    template<typename SIGNAL, typename RECEIVER, typename... TEST_FUNC,
             typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value > 0)>>
    Connection connect_signal(SIGNAL& signal, void (RECEIVER::*method)(void), TEST_FUNC&&... test_func)
    {
        constexpr auto signal_size = std::tuple_size<typename std::decay<typename SIGNAL::Signature>::type>::value;
        return _connect_signal(signal, _connection_helper(signal, method, std::make_index_sequence<signal_size>{}),
                               std::forward<TEST_FUNC>(test_func)...);
    }

    /** Argument-ignoring overload for CONST functions (see overload above). */
    template<typename SIGNAL, typename RECEIVER, typename... TEST_FUNC,
             typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value > 0)>>
    Connection connect_signal(SIGNAL& signal, void (RECEIVER::*method)(void) const, TEST_FUNC&&... test_func)
    {
        constexpr auto signal_size = std::tuple_size<typename std::decay<typename SIGNAL::Signature>::type>::value;
        return _connect_signal(signal, _connection_helper(signal, method, std::make_index_sequence<signal_size>{}),
                               std::forward<TEST_FUNC>(test_func)...);
    }

    /** Checks if a particular Connection is managed by this object. */
    bool has_connection(const Connection& other) const
    {
        for (const Connection& connection : m_connections) {
            if (connection == other) {
                return true;
            }
        }
        return false;
    }

    /** All Connections managed by this object. */
    const std::vector<Connection>& get_connections() const
    {
        _cleanup();
        return m_connections;
    }

    /** (Re-)Enables all tracked Connections managed by this object. */
    void enable_all_connections()
    {
        for (Connection& connection : m_connections) {
            connection.enable();
        }
    }

    /** Disables all Connections managed by this object. */
    void disable_all_connections()
    {
        for (Connection& connection : m_connections) {
            connection.disable();
        }
    }

    /** Disconnects all Signals from Connections managed by this object. */
    void disconnect_all_connections()
    {
        for (Connection& connection : m_connections) {
            connection.disconnect();
        }
    }

private: // methods
    /** Removes all disconnected Connections. */
    void _cleanup() const
    {
        std::remove_if(m_connections.begin(), m_connections.end(),
                       [](const Connection& connection) -> bool { return !connection.is_connected(); });
    }

    /** Connects a new Connection to be managed by this object. */
    template<typename SIGNAL, typename... ARGS>
    Connection _connect_signal(SIGNAL& signal, ARGS&&... args)
    {
        _cleanup();
        Connection connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.push_back(connection);
        return connection;
    }

    /** Helper function for the 3rd overload of `connect_signal`.
     * Returns a lambda wrapping a call from any Signal to a method of this object that takes no arguments.
     */
    template<typename SIGNAL, typename RECEIVER, std::size_t... index>
    decltype(auto) _connection_helper(SIGNAL&, void (RECEIVER::*method)(void), std::index_sequence<index...>)
    {
        return [this, method](typename std::tuple_element<index, typename SIGNAL::Signature>::type...) {
            (static_cast<RECEIVER*>(this)->*method)();
        };
    }

private: // fields
    /** All incoming Connections. */
    mutable std::vector<Connection> m_connections;
};

/**********************************************************************************************************************/

} // namespace notf
