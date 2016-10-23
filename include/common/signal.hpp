#pragma once

#include <assert.h>
#include <memory>
#include <vector>

// see commit de6d173f0339fadc20f128891ecaba6f637e07cb for a thread-safe (but only 16-25% as fast) implementation

namespace notf {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief A connection between a signal and a callback.
struct Connection {

    template <typename...>
    friend class Signal;

private: //struct
    /// @brief Data block shared by two Connection instances.
    struct Data {
        /// @brief Is the connection still active?
        bool is_connected{true};
    };

private: // methods for Signal
    /// @brief Value constructor.
    ///
    /// @param data Shared Connection data block.
    Connection(std::shared_ptr<Data> data)
        : m_data(std::move(data))
    {
    }

    /// @brief Creates a new, default constructed Connections object.
    static Connection make_connection()
    {
        return Connection(std::make_shared<Data>());
    }

public: // methods
    Connection(const Connection&) = default;
    Connection& operator=(const Connection&) = default;
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    /// @brief Check if the connection is alive.
    ///
    /// @return True if the connection is alive.
    bool is_connected() const { return m_data && m_data->is_connected; }

    /// @brief Breaks this Connection.
    ///
    /// After calling this function, future signals will not be delivered.
    void disconnect()
    {
        if (!m_data) {
            return;
        }
        m_data->is_connected = false;
    }

private: // fields
    /// @brief Data block shared by two Connection instances.
    std::shared_ptr<Data> m_data;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Manager class owned by instances that have methods connected to Signals.
///
/// The Callback Manager tracks all Connections representing Signal Targets to a member function of an object,
/// and disconnects them when that object goes out of scope.
/// The Callback Manager member should be placed at the end of the class definition, so it is destructed before any other.
/// This way, all data required for the last remaining calls to finish is still valid.
/// If used within a class hierarchy, the most specialized class has the responsibility to disconnect all of its base
/// class' Signals before destroying any other members.
class CallbackManager {

public: // methods
    /// @brief Default Constructor.
    CallbackManager() = default;

    CallbackManager(const CallbackManager&) = delete;
    CallbackManager& operator=(const CallbackManager&) = delete;
    CallbackManager(CallbackManager&&) = delete;
    CallbackManager& operator=(CallbackManager&&) = delete;

    /// @brief Destructor.
    ~CallbackManager() { disconnect_all(); }

    /// @brief Creates and tracks a new Connection between the Signal and a lambda or free function.
    ///
    /// @brief signal       The Signal to connect to.
    /// @brief lambda       Function object (lambda / free function) to call.
    /// @brief test_func    (optional) Test function.
    template <typename SIGNAL, typename... ARGS>
    void connect(SIGNAL& signal, ARGS&&... args)
    {
        Connection connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.emplace_back(std::move(connection));
    }

    /// @brief Disconnects all tracked Connections.
    void disconnect_all()
    {
        for (Connection& connection : m_connections) {
            connection.disconnect();
        }
        m_connections.clear();
    }

private: // fields
    /// @brief All managed Connections.
    std::vector<Connection> m_connections;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief An object capable of firing (emitting) signals to connected targets.
template <typename... SIGNATURE>
class Signal {

    friend class CallbackManager;

private: // struct
    /// @brief Connection and target function pair.
    struct Target {
        Target(Connection connection, std::function<void(SIGNATURE...)> function,
               std::function<bool(SIGNATURE...)> test_function = [](SIGNATURE...) { return true; })
            : connection(std::move(connection))
            , function(std::move(function))
            , test_function(std::move(test_function))
        {
        }

        /// @brief Connection through which the Callback is performed.
        Connection connection;

        /// @brief Callback function.
        std::function<void(SIGNATURE...)> function;

        /// @brief The signal is only passed over this Connection if this function evaluates to true.
        std::function<bool(SIGNATURE...)> test_function;
    };

public: // methods
    /// @brief Default constructor.
    Signal()
        : m_targets()
    {
    }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// @brief Destructor.
    ~Signal() { disconnect_all(); }

    /// @brief Move Constructor.
    ///
    /// @param other
    Signal(Signal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

    /// @brief RValue assignment Operator.
    ///
    /// @param other
    ///
    /// @return This instance.
    Signal& operator=(Signal&& other) noexcept
    {
        m_targets = std::move(other.m_targets);
        return *this;
    }

    /// @brief Connects a new target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new target is connected.
    ///
    /// @param function     Target function.
    /// @param test_func    (optional) Test function. Target is always called when empty.
    ///
    /// @return The created Connection.
    Connection connect(std::function<void(SIGNATURE...)> function,
                       std::function<bool(SIGNATURE...)> test_func = {})
    {
        assert(function);

        // create a new target vector (will be filled in with
        // the existing and still active targets within the lock below)
        auto new_targets = std::vector<Target>();

        // copy existing, connected targets
        new_targets.reserve(m_targets.size() + 1);
        for (const auto& target : m_targets) {
            if (target.connection.is_connected()) {
                new_targets.push_back(target);
            }
        }

        // add the new connection to the new vector
        Connection connection = Connection::make_connection();
        if (test_func) {
            new_targets.emplace_back(connection, std::move(function), std::move(test_func));
        }
        else {
            new_targets.emplace_back(connection, std::move(function));
        }

        // replace the stored targets
        std::swap(m_targets, new_targets);

        return connection;
    }

    /// @brief Disconnect all Connections from this Signal.
    void disconnect_all()
    {
        // disconnect all callbacks
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
    }

    /// @brief Fires (emits) the signal.
    ///
    /// Arguments to this function are passed to the connected callbacks and must match the signature with which the
    /// Signal instance was defined.
    /// Build-in type casting works as expected (float -> int etc.).
    template <typename... ARGUMENTS>
    void operator()(ARGUMENTS&&... args) const
    {
        for (auto& target : m_targets) {
            if (!target.connection.is_connected() || !target.test_function(args...)) {
                continue;
            }
            target.function(args...);
        }
    }

private: // methods for Connections
    /// @brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
    /// @param obj          Instance providing the callback.
    /// @param method       Address of the method.
    /// @param test_func    (optional) Test function. Callback is always called when empty.
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
    /// @brief All targets of this Signal.
    std::vector<Target> m_targets;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Full specialization for Signals that require no arguments.
template <>
class Signal<> {

    friend class CallbackManager;

private: // struct
    /// @brief Connection and target function pair.
    struct Target {
        Target(Connection connection, std::function<void()> function)
            : connection(std::move(connection))
            , function(std::move(function))
        {
        }

        /// @brief Connection through which the Callback is performed.
        Connection connection;

        /// @brief Callback function.
        std::function<void()> function;
    };

public: // methods
    /// @brief Default constructor.
    Signal()
        : m_targets()
    {
    }

    /// @brief Destructor.
    ~Signal()
    {
        disconnect_all();
    }

    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// @brief Move Constructor.
    ///
    /// @param other
    Signal(Signal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

    /// @brief RValue assignment Operator.
    ///
    /// @param other
    ///
    /// @return This instance.
    Signal& operator=(Signal&& other) noexcept
    {
        m_targets = std::move(other.m_targets);
        return *this;
    }

    /// @brief Connects a new target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new target is connected.
    ///
    /// @param function     Target function.
    ///
    /// @return The created Connection.
    Connection connect(std::function<void()> function)
    {
        assert(function);

        // create a new target vector (will be filled in with
        // the existing and still active targets within the lock below)
        auto new_targets = std::vector<Target>();

        // copy existing, connected targets
        new_targets.reserve(m_targets.size() + 1);
        for (const auto& target : m_targets) {
            if (target.connection.is_connected()) {
                new_targets.push_back(target);
            }
        }

        // add the new connection to the new vector
        Connection connection = Connection::make_connection();
        new_targets.emplace_back(connection, std::move(function));

        // replace the stored targets
        std::swap(m_targets, new_targets);

        return connection;
    }

    /// @brief Disconnect all Connections from this Signal.
    void disconnect_all()
    {
        // disconnect all callbacks
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
    }

    /// @brief Fires (emits) the signal.
    void operator()() const
    {
        for (auto& target : m_targets) {
            if (!target.connection.is_connected()) {
                continue;
            }
            target.function();
        }
    }

private: // methods for Connections
    /// @brief Overload of connect() to connect to member functions.
    ///
    /// Creates and stores a lambda function to access the member.
    ///
    /// @param obj          Instance providing the callback.
    /// @param method       Address of the method.
    /// @param test_func    (optional) Test function. Callback is always called when empty.
    template <typename OBJ>
    Connection connect(OBJ* obj, void (OBJ::*method)())
    {
        assert(obj);
        assert(method);
        return connect([obj, method]() { (obj->*method)(); });
    }

private: // fields
    /// @brief All targets of this Signal.
    std::vector<Target> m_targets;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Curiously recurring template pattern mixin class to provide Signals to SUBCLASS.
template <typename SUBCLASS>
class Signaler {

public: // methods
    /// @brief Creates a connection managed by the object connecting the given signal to a function object.
    template <typename SIGNAL, typename... ARGS>
    void connect(SIGNAL& signal, ARGS&&... args) { m_callback_manager.connect(signal, std::forward<ARGS>(args)...); }

    /// @brief Creates a Connection connecting the given Signal to a member function of this Object.
    template <typename SIGNAL, typename... SIGNATURE, typename... TEST_FUNC>
    void connect(SIGNAL& signal, void (SUBCLASS::*method)(SIGNATURE...), TEST_FUNC&&... test_func)
    {
        m_callback_manager.connect(signal, this, std::forward<decltype(method)>(method), std::forward<TEST_FUNC>(test_func)...);
    }

    /// @brief Disconnects all Signals from Connections managed by this object.
    void disconnect_all() { m_callback_manager.disconnect_all(); }

private: // fields
    // @brief Callback manager owning one half of the Signals' Connections.
    CallbackManager m_callback_manager;
};

} // namespace notf

#if 0
#include <chrono>
#include <iostream>

#include "signal.hpp"
using namespace notf;

using Clock = std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

/// @brief Free signal receiver.
/// @param value    Value to add
void freeCallback(uint value)
{
    static ulong counter = 0;
    counter += value;
}

void emptyCallback()
{
    static ulong counter = 0;
    counter += 1;
}

/// @brief Free signal receiver for testing values.
/// @param value    Value to print
void freePrintCallback(uint value)
{
    std::cout << "Free print callback: " << value << "\n";
}

/// @brief Simple class with a signal.
class Sender {
public:
    void setValue(uint value) { valueChanged(value); }
    void fireEmpty() { emptySignal(); }
    Signal<uint> valueChanged;
    Signal<> emptySignal;
};

/// @brief Simple receiver class providing two slots.
class Receiver : public Signaler<Receiver>{
public:
    void memberCallback(uint value) { counter += value; }
    void memberPrintCallback(uint value) { std::cout << "valueChanged: " << value << "\n"; }
    ulong counter = 0;
};

int main()
{
    ulong REPETITIONS = 10000000;

    Sender sender;
    //    sender.counterIncreased.connect(freeCallback);
    sender.emptySignal.connect(emptyCallback);

    Receiver receiver;
    receiver.connect(sender.valueChanged, &Receiver::memberCallback);

    Clock::time_point t0 = Clock::now();
    for (uint i = 0; i < REPETITIONS; ++i) {
        sender.setValue(1);
        sender.fireEmpty();
        emptyCallback();
    }
    Clock::time_point t1 = Clock::now();

    milliseconds ms = std::chrono::duration_cast<milliseconds>(t1 - t0);
    auto time_delta = ms.count();
    if (time_delta == 0) {
        time_delta = 1;
    }
    ulong throughput = REPETITIONS / static_cast<ulong>(time_delta);

    std::cout << "Throughput with " << REPETITIONS << " repetitions: " << throughput << "/ms" << std::endl;
    // throughput is <= 333333/ms on a release build with a single receiver and 10000000 repetitions, single-threaded, no arguments
    // throughput is <= 200000/ms on a release build with a single receiver and 10000000 repetitions, single-threaded, argument
    // throughput is <=  58850/ms on a release build with a single receiver and 10000000 repetitions, multi-threaded, no arguments
    // throughput is <=  55555/ms on a release build with a single receiver and 10000000 repetitions, multi-threaded, argument

    return 0;
}

#endif
