#pragma once

#include <algorithm>
#include <assert.h>
#include <memory>
#include <vector>

// see commit de6d173f0339fadc20f128891ecaba6f637e07cb for a thread-safe (but only 16-25% as fast) implementation

namespace notf {

/**********************************************************************************************************************/

/** A connection between a signal and a callback. */
struct Connection {

    template <typename...>
    friend class Signal;

private: //struct
    /** Data block shared by two Connection instances. */
    struct Data {
        /** Is the connection still active? */
        bool is_connected = true;

        /** Is the connection currently enabled? */
        bool is_enabled = true;
    };

private: // methods for Signal
    /** @param data Shared Connection data block. */
    Connection(std::shared_ptr<Data> data)
        : m_data(std::move(data))
    {
    }

    /** Creates a new, default constructed Connection object. */
    static Connection create()
    {
        return Connection(std::make_shared<Data>());
    }

public: // methods
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

private: // fields
    /** Data block shared by two Connection instances. */
    std::shared_ptr<Data> m_data;
};

/**********************************************************************************************************************/

/** Manager object owned by objects that receive Signals.
 * The CallbackManager tracks all incoming Connections to member functions of the owner and disconnects them, when the
 * owner is destructed.
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
    void connect(SIGNAL& signal, ARGS&&... args)
    {
        Connection connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.emplace_back(std::move(connection));
    }

    /** Temporarily disables all tracked Connections. */
    void disable_all()
    {
        for (auto& connection : m_connections) {
            connection.disable();
        }
    }

    /** (Re-)Enables all tracked Connections. */
    void enable_all()
    {
        for (auto& connection : m_connections) {
            connection.enable();
        }
    }

    /** Disconnects all tracked Connections. */
    void disconnect_all()
    {
        for (Connection& connection : m_connections) {
            connection.disconnect();
        }
        m_connections.clear();
    }

private: // fields
    /** All managed Connections. */
    std::vector<Connection> m_connections;
};

/**********************************************************************************************************************/

/** An object capable of firing (emitting) signals to connected targets. */
template <typename... SIGNATURE>
class Signal {

    friend class CallbackManager;

private: // struct
    /** Connection and target function pair. */
    struct Target {
        Target(Connection connection, std::function<void(SIGNATURE...)> function,
               std::function<bool(SIGNATURE...)> test_function)
            : connection(std::move(connection))
            , function(std::move(function))
            , test_function(std::move(test_function))
        {
        }

        /** Connection through which the Callback is performed. */
        Connection connection;

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
    Connection connect(std::function<void(SIGNATURE...)> function,
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
        Connection connection = Connection::create();
        m_targets.emplace_back(connection, std::move(function), std::move(test_func));

        return connection;
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (auto& target : m_targets) {
            target.connection.disable();
        }
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (auto& target : m_targets) {
            target.connection.enable();
        }
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
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

private: // methods for CallbackManager
    /** Overload of connect() to connect to member functions.
     * Creates and stores a lambda function to access the member.
     * @param obj          Instance providing the callback.
     * @param method       Address of the method.
     * @param test_func    (optional) Test function. Callback is always called when empty.
     */
    template <typename OBJ>
    Connection connect(OBJ* obj, void (OBJ::*method)(SIGNATURE... args), std::function<bool(SIGNATURE...)>&& test_func = {})
    {
        assert(obj);
        assert(method);
        return connect(
            [obj, method](SIGNATURE... args) { (obj->*method)(args...); },
            std::forward<std::function<bool(SIGNATURE...)>>(test_func));
    }

private: // fields
    /** All targets of this Signal. */
    std::vector<Target> m_targets;
};

/**********************************************************************************************************************/

/** Full specialization for Signals that require no arguments.
 * Since they don't have a test-function, emitting Signals without arguments is about 1/3 faster than those with args.
 */
template <>
class Signal<> {

    friend class CallbackManager;

private: // struct
    /** Connection and target function pair. */
    struct Target {
        Target(Connection connection, std::function<void()> function)
            : connection(std::move(connection))
            , function(std::move(function))
        {
        }

        /** Connection through which the Callback is performed. */
        Connection connection;

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
    Connection connect(std::function<void()> function)
    {
        assert(function);

        // remove disconnected targets
        std::remove_if(m_targets.begin(), m_targets.end(), [](const Target& target) -> bool {
            return !target.connection.is_connected();
        });

        // create a new Connection
        Connection connection = Connection::create();
        m_targets.emplace_back(connection, std::move(function));

        return connection;
    }

    /** Temporarily disables all Connections of this Signal. */
    void disable()
    {
        for (auto& target : m_targets) {
            target.connection.disable();
        }
    }

    /** (Re-)Enables all Connections of this Signal. */
    void enable()
    {
        for (auto& target : m_targets) {
            target.connection.enable();
        }
    }

    /** Disconnect all Connections from this Signal. */
    void disconnect()
    {
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
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

private: // methods for CallbackManager
    /** Overload of connect() to connect to member functions.
     * Creates and stores a lambda function to access the member.
     * @param obj          Instance providing the callback.
     * @param method       Address of the method.
     */
    template <typename OBJ>
    Connection connect(OBJ* obj, void (OBJ::*method)())
    {
        assert(obj);
        assert(method);
        return connect([obj, method]() { (obj->*method)(); });
    }

private: // fields
    /** All targets of this Signal. */
    std::vector<Target> m_targets;
};

/**********************************************************************************************************************/

/** Curiously recurring template pattern mixin class to be able to receive Signals to SUBCLASS. */
template <typename Subclass>
class provide_slots {

public: // methods
    /** Creates a connection managed by this object, connecting the given signal to a function object. */
    template <typename SIGNAL, typename... ARGS>
    void connect_slot(SIGNAL& signal, ARGS&&... args) { m_callback_manager.connect(signal, std::forward<ARGS>(args)...); }

    /** Creates a Connection connecting the given Signal to a member function of this object. */
    template <typename SIGNAL, typename... SIGNATURE, typename... TEST_FUNC>
    void connect_slot(SIGNAL& signal, void (Subclass::*method)(SIGNATURE...), TEST_FUNC&&... test_func)
    {
        m_callback_manager.connect(signal,
                                   static_cast<Subclass*>(this),
                                   std::forward<decltype(method)>(method),
                                   std::forward<TEST_FUNC>(test_func)...);
    }

    /** Temporarily disables all Connections into this object. */
    void disable_all_slots() { m_callback_manager.disable_all(); }

    /** (Re-)Enables all tracked Connections into this object. */
    void enable_all_slots() { m_callback_manager.enable_all(); }

    /** Disconnects all Signals from Connections managed by this object. */
    void disconnect_all_slots() { m_callback_manager.disconnect_all(); }

private: // fields
    /** Callback manager owning one half of the Signals' Connections. */
    CallbackManager m_callback_manager;
};

} // namespace notf
