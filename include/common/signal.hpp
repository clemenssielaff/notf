#pragma once

#include <algorithm>
#include <assert.h>
#include <exception>
#include <memory>
#include <vector>

// see commit de6d173f0339fadc20f128891ecaba6f637e07cb for a thread-safe (but only 16-25% as fast) implementation

namespace notf {

/** Every connection from a Signal has an application-unique ID. */
using ConnectionID = size_t;

namespace detail {

    /******************************************************************************************************************/

    /** A connection between a signal and a callback. */
    struct Connection {

        template <typename...>
        friend class Signal;

    private: //struct
        /** Data block shared by two Connection instances. */
        struct Data {
            Data(bool is_connected, bool is_enabled, ConnectionID id)
                : is_connected(std::move(is_connected))
                , is_enabled(std::move(is_enabled))
                , id(std::move(id))
            {
            }

            /** Is the connection still active? */
            bool is_connected;

            /** Is the connection currently enabled? */
            bool is_enabled;

            /** ID of this connection, is application-unique. */
            const ConnectionID id;
        };

    public: // methods
        /** Creates a new, default constructed Connection object. */
        static Connection create()
        {
            return Connection(std::make_shared<Data>(true, true, get_next_id()));
        }

        Connection(const Connection&) = default;
        Connection& operator=(const Connection&) = default;
        Connection(Connection&&) = default;
        Connection& operator=(Connection&&) = default;

        /** Check if the connection is alive. */
        bool is_connected() const { return m_data && m_data->is_connected; }

        /** Checks if the Connection is currently enabled. */
        bool is_enabled() const { return m_data && m_data->is_enabled; }

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

        /** Returns the ID of this Connection or 0 when disconnected. */
        ConnectionID get_id() const
        {
            if (m_data) {
                return m_data->id;
            }
            return 0;
        }

    public: // static methods
        /** The next Connection ID. */
        static ConnectionID get_next_id()
        {
            static ConnectionID next_id = 0;
            return ++next_id; // leaving 0 as 'invalid'
        }

    private: // methods
        /** @param data Shared Connection data block. */
        explicit Connection(std::shared_ptr<Data> data)
            : m_data(std::move(data))
        {
        }

    private: // fields
        /** Data block shared by two Connection instances. */
        std::shared_ptr<Data> m_data;
    };

    /******************************************************************************************************************/

    /** Manager object owned by objects that receive Signals.
     * The CallbackManager tracks all incoming Connections to member functions of the owner and disconnects them, when
     * the owner is destructed.
     */
    class CallbackManager {

    public: // methods
        CallbackManager() = default;

        ~CallbackManager() { disconnect_all(); }

        // no copy, move & -assignment
        CallbackManager(const CallbackManager&) = delete;
        CallbackManager& operator=(const CallbackManager&) = delete;
        CallbackManager(CallbackManager&&) = delete;
        CallbackManager& operator=(CallbackManager&&) = delete;

        /** Creates and tracks a new Connection between the Signal and a lambda or free function.
         * @brief signal       The Signal to connect to.
         * @brief lambda       Function object (lambda / free function) to call.
         * @brief test_func    (optional) Test function.
         */
        template <typename SIGNAL, typename... ARGS>
        ConnectionID connect(SIGNAL& signal, ARGS&&... args)
        {
            Connection connection = signal.connect(std::forward<ARGS>(args)...);
            ConnectionID id = connection.get_id();
            m_connections.emplace_back(std::move(connection));
            return id;
        }

        /** Checks if a particular Connection is managed by this manager. */
        bool has_connection(const ConnectionID id) const
        {
            if (!id) {
                return false;
            }
            for (const Connection& connection : m_connections) {
                if (connection.get_id() == id) {
                    return true;
                }
            }
            return false;
        }

        /** Returns a vector of all managed (connected) Connections.*/
        std::vector<ConnectionID> get_connections() const
        {
            std::vector<ConnectionID> result;
            result.reserve(m_connections.size());
            for (const Connection& connection : m_connections) {
                ConnectionID id = connection.get_id();
                if (id) {
                    result.push_back(id);
                }
            }
            return result;
        }

        /** Temporarily disables all tracked Connections. */
        void disable_all()
        {
            for (auto& connection : m_connections) {
                connection.disable();
            }
        }

        /** Disables a specific Connection into the managed object.
         * @param connection            ID of the Connection.
         * @throw std:runtime_error     If there is no Connection with the given ID connected.
         */
        void disable(const ConnectionID id)
        {
            for (auto& connection : m_connections) {
                if (connection.get_id() == id) {
                    connection.disable();
                    return;
                }
            }
            throw std::runtime_error("Cannot disable unknown connection");
        }

        /** (Re-)Enables all tracked Connections. */
        void enable_all()
        {
            for (auto& connection : m_connections) {
                connection.enable();
            }
        }

        /** Enables a specific Connection into the managed object.
         * @param connection            ID of the Connection.
         * @throw std:runtime_error     If there is no Connection with the given ID connected.
         */
        void enable(const ConnectionID id)
        {
            for (auto& connection : m_connections) {
                if (connection.get_id() == id) {
                    connection.enable();
                    return;
                }
            }
            throw std::runtime_error("Cannot enable unknown connection");
        }

        /** Disconnects all tracked Connections. */
        void disconnect_all()
        {
            for (Connection& connection : m_connections) {
                connection.disconnect();
            }
            m_connections.clear();
        }

        /** Disconnects a specific Connection into the managed object.
         * @param connection            ID of the Connection.
         * @throw std:runtime_error     If there is no Connection with the given ID connected.
         */
        void disconnect(const ConnectionID id)
        {
            for (auto& connection : m_connections) {
                if (connection.get_id() == id) {
                    connection.disconnect();
                    return;
                }
            }
            throw std::runtime_error("Cannot diconnected unknown connection");
        }

    private: // fields
        /** All managed Connections. */
        std::vector<Connection> m_connections;
    };

} // namespace detail

/**********************************************************************************************************************/

/** An object capable of firing (emitting) signals to connected targets. */
template <typename... SIGNATURE>
class Signal {

public: // type
    using Signature = std::tuple<SIGNATURE...>;

private: // struct
    /** Connection and target function pair. */
    struct Target {
        Target(detail::Connection connection, std::function<void(SIGNATURE...)> function,
               std::function<bool(SIGNATURE...)> test_function)
            : connection(std::move(connection))
            , function(std::move(function))
            , test_function(std::move(test_function))
        {
        }

        /** Connection through which the Callback is performed. */
        detail::Connection connection;

        /** Callback function. */
        std::function<void(SIGNATURE...)> function;

        /** The signal is only passed over this Connection if this function evaluates to true. */
        std::function<bool(SIGNATURE...)> test_function;
    };

public: // methods
    Signal() = default;

    // no copy / assignment
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    ~Signal() { disconnect(); }

    /** Move Constructor. */
    Signal(Signal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

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
    detail::Connection connect(std::function<void(SIGNATURE...)> function,
                               std::function<bool(SIGNATURE...)> test_func = {})
    {
        assert(function);

        // default test succeeds always
        if (!test_func) {
            test_func = [](SIGNATURE...) { return true; };
        }

        // remove disconnected targets
        std::remove_if(m_targets.begin(), m_targets.end(), [](const Target& target) -> bool {
            return !target.connection.is_connected();
        });

        // add the new connection
        detail::Connection connection = detail::Connection::create();
        m_targets.emplace_back(connection, std::move(function), std::move(test_func));

        return connection;
    }

    /** Checks if a particular Connection is connected to this Signal. */
    bool has_connection(const ConnectionID id) const
    {
        if (!id) {
            return false;
        }
        for (const Target& target : m_targets) {
            if (target.connection.get_id() == id) {
                return true;
            }
        }
        return false;
    }

    /** Returns a vector of all (connected) Connections.*/
    std::vector<ConnectionID> get_connections() const
    {
        std::vector<ConnectionID> result;
        result.reserve(m_targets.size());
        for (const Target& target : m_targets) {
            ConnectionID id = target.connection.get_id();
            if (id) {
                result.push_back(id);
            }
        }
        return result;
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (auto& target : m_targets) {
            target.connection.disable();
        }
    }

    /** Disables a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void disable(const ConnectionID id)
    {
        for (auto& target : m_targets) {
            if (target.connection.get_id() == id) {
                target.connection.disable();
                return;
            }
        }
        throw std::runtime_error("Cannot disable unknown connection");
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (auto& target : m_targets) {
            target.connection.enable();
        }
    }

    /** Enables a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void enable(const ConnectionID id)
    {
        for (auto& target : m_targets) {
            if (target.connection.get_id() == id) {
                target.connection.enable();
                return;
            }
        }
        throw std::runtime_error("Cannot enable unknown connection");
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
        m_targets.clear();
    }

    /** Disconnects a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void disconnect(const ConnectionID id)
    {
        for (auto& target : m_targets) {
            if (target.connection.get_id() == id) {
                target.connection.disconnect();
                return;
            }
        }
        throw std::runtime_error("Cannot disconnect unknown connection");
    }

    /** Fires (emits) the signal.
     * Arguments to this function are passed to the connected callbacks and must match the signature with which the
     * Signal instance was defined.
     * Build-in type casting works as expected (float -> int etc.).
     */
    template <typename... ARGUMENTS>
    void operator()(ARGUMENTS&&... args) const
    {
        for (auto& target : m_targets) {
            if (true
                && target.connection.is_connected()
                && target.connection.is_enabled()
                && target.test_function(args...)) {
                target.function(args...);
            }
        }
    }

private: // fields
    /** All targets of this Signal. */
    std::vector<Target> m_targets;
};

/**********************************************************************************************************************/

/** Full specialization for Signals that require no arguments.
 * Since they don't have a test-function, emitting Signals without arguments is 1.65x as fast than those with args.
 */
template <>
class Signal<> {

public: // type
    using Signature = std::tuple<>;

private: // struct
    /** Connection and target function pair. */
    struct Target {
        Target(detail::Connection connection, std::function<void()> function)
            : connection(std::move(connection))
            , function(std::move(function))
        {
        }

        /** Connection through which the Callback is performed. */
        detail::Connection connection;

        /** Callback function. */
        std::function<void()> function;
    };

public: // methods
    Signal() = default;

    ~Signal() { disconnect(); }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /** Move Constructor. */
    Signal(Signal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

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
    detail::Connection connect(std::function<void()> function)
    {
        assert(function);

        // remove disconnected targets
        std::remove_if(m_targets.begin(), m_targets.end(), [](const Target& target) -> bool {
            return !target.connection.is_connected();
        });

        // create a new Connection
        detail::Connection connection = detail::Connection::create();
        m_targets.emplace_back(connection, std::move(function));

        return connection;
    }

    /** Checks if a particular Connection is connected to this Signal. */
    bool has_connection(const ConnectionID id) const
    {
        if (!id) {
            return false;
        }
        for (const Target& target : m_targets) {
            if (target.connection.get_id() == id) {
                return true;
            }
        }
        return false;
    }

    /** Returns a vector of all (connected) Connections.*/
    std::vector<ConnectionID> get_connections() const
    {
        std::vector<ConnectionID> result;
        result.reserve(m_targets.size());
        for (const Target& target : m_targets) {
            ConnectionID id = target.connection.get_id();
            if (id) {
                result.push_back(id);
            }
        }
        return result;
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (auto& target : m_targets) {
            target.connection.disable();
        }
    }

    /** Disables a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void disable(const ConnectionID id)
    {
        for (auto& target : m_targets) {
            if (target.connection.get_id() == id) {
                target.connection.disable();
                return;
            }
        }
        throw std::runtime_error("Cannot disable unknown connection");
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (auto& target : m_targets) {
            target.connection.enable();
        }
    }

    /** Enables a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void enable(const ConnectionID id)
    {
        for (auto& target : m_targets) {
            if (target.connection.get_id() == id) {
                target.connection.enable();
                return;
            }
        }
        throw std::runtime_error("Cannot disable unknown connection");
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
        m_targets.clear();
    }

    /** Disconnects a specific Connection of this Signal.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected to this Signal.
     */
    void disconnect(const ConnectionID id)
    {
        for (auto& target : m_targets) {
            if (target.connection.get_id() == id) {
                target.connection.disconnect();
                return;
            }
        }
        throw std::runtime_error("Cannot disconnect unknown connection");
    }

    /** Fires (emits) the signal. */
    void operator()() const
    {
        for (auto& target : m_targets) {
            if (true
                && target.connection.is_connected()
                && target.connection.is_enabled()) {
                target.function();
            }
        }
    }

private: // fields
    /** All targets of this Signal. */
    std::vector<Target> m_targets;
};

/**********************************************************************************************************************/

/** Mixin class to be able to receive Signals. */
class receive_signals {

public: // methods
    /** Creates a connection managed by this object, connecting the given signal to a function object. */
    template <typename SIGNAL, typename... ARGS>
    ConnectionID connect_signal(SIGNAL& signal, ARGS&&... args)
    {
        return m_callback_manager.connect(signal, std::forward<ARGS>(args)...);
    }

    /** Creates a Connection connecting the given Signal to a member function of this object. */
    template <typename SIGNAL, typename RECEIVER, typename... SIGNATURE, typename... ARGS,
              typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value == sizeof... (SIGNATURE))>>
    ConnectionID connect_signal(SIGNAL& signal, void (RECEIVER::*method)(SIGNATURE...), ARGS&&... args)
    {
        return m_callback_manager.connect(
            signal,
            [this, method](SIGNATURE... args) { (static_cast<RECEIVER*>(this)->*method)(args...); },
            std::forward<ARGS>(args)...);
    }

    /** Creates a Connection connecting the given Signal to a const member function of this object. */
    template <typename SIGNAL, typename RECEIVER, typename... SIGNATURE, typename... ARGS,
              typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value == sizeof... (SIGNATURE))>>
    ConnectionID connect_signal(SIGNAL& signal, void (RECEIVER::*method)(SIGNATURE...) const, ARGS&&... args)
    {
        return m_callback_manager.connect(
                    signal,
                    [this, method](SIGNATURE... args) { (static_cast<RECEIVER*>(this)->*method)(args...); },
        std::forward<ARGS>(args)...);
    }

    /** Overload to connect any Signal to a member function without arguments.
     * This way, you can connect a `Signal<int>` to a function `foo()` without having to manually wrap the function
     * call in a lambda that discards the additional arguments.
     */
    template <typename SIGNAL, typename RECEIVER, typename... TEST_FUNC,
              typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value > 0)>>
    ConnectionID connect_signal(SIGNAL& signal, void (RECEIVER::*method)(void), TEST_FUNC&&... test_func)
    {
        constexpr auto signal_size = std::tuple_size<typename std::decay<typename SIGNAL::Signature>::type>::value;
        return m_callback_manager.connect(signal,
                                          _connection_helper(signal, method, std::make_index_sequence<signal_size>{}),
                                          std::forward<TEST_FUNC>(test_func)...);
    }

    /** Argument-ignoring overload for const functions (see overload above). */
    template <typename SIGNAL, typename RECEIVER, typename... TEST_FUNC,
              typename = std::enable_if_t<(std::tuple_size<typename SIGNAL::Signature>::value > 0)>>
    ConnectionID connect_signal(SIGNAL& signal, void (RECEIVER::*method)(void) const, TEST_FUNC&&... test_func)
    {
    constexpr auto signal_size = std::tuple_size<typename std::decay<typename SIGNAL::Signature>::type>::value;
    return m_callback_manager.connect(signal,
                                      _connection_helper(signal, method, std::make_index_sequence<signal_size>{}),
                                      std::forward<TEST_FUNC>(test_func)...);
    }

    /** Checks if a particular Connection is connected to this object. */
    bool has_connection(const ConnectionID id) const { return m_callback_manager.has_connection(id); }

    /** Returns a vector of all (connected) Connections to this object.*/
    std::vector<ConnectionID> get_connections() const { return m_callback_manager.get_connections(); }

    /** Temporarily disables all Connections into this object. */
    void disable_all_connections() { m_callback_manager.disable_all(); }

    /** Disables a specific Connection into this object.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected.
     */
    void disable_connection(const ConnectionID id) { m_callback_manager.disable(std::move(id)); }

    /** (Re-)Enables all tracked Connections into this object. */
    void enable_all_connections() { m_callback_manager.enable_all(); }

    /** Enables a specific Connection into this object.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected.
     */
    void enable_connection(const ConnectionID id) { m_callback_manager.enable(std::move(id)); }

    /** Disconnects all Signals from Connections managed by this object. */
    void disconnect_all_connections() { m_callback_manager.disconnect_all(); }

    /** Disconnects a specific Connection into this object.
     * @param connection            ID of the Connection.
     * @throw std:runtime_error     If there is no Connection with the given ID connected.
     */
    void disconnect_connection(const ConnectionID id) { m_callback_manager.disconnect(std::move(id)); }

private: // methods
    /** Helper function for the third overload of `connect_signal`.
     * Returns a lambda wrapping a call from any Signal to a method of this object that takes no arguments.
     */
    template <typename SIGNAL, typename RECEIVER, std::size_t... index>
    decltype(auto) _connection_helper(SIGNAL&, void (RECEIVER::*method)(void), std::index_sequence<index...>)
    {
        return [this, method](typename std::tuple_element<index, typename SIGNAL::Signature>::type...) {
            (static_cast<RECEIVER*>(this)->*method)();
        };
    }

private: // fields
    /** Callback manager owning one half of the Signals' Connections. */
    detail::CallbackManager m_callback_manager;
};

} // namespace notf

// TODO: connection object instead of connectionId so that you can say self.white_on_click.disable()
// instead of self.on_mouse_button.disable(self._white_on_click) ... or maken't that sense?
