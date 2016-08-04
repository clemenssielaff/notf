#pragma once

/// Signals are dynamic Signal-Callback connections between different objects in the code.
///
/// There is nothing stoping you from creating circular callbacks with these, so be careful not to do that.
/// If you do it by accident - you'll know.

#include <assert.h>
#include <memory>
#include <vector>

namespace signal {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief A connection between a signal and a callback.
struct Connection {

    template <typename...>
    friend class Signal;

private: //struct
    /// \brief Data block shared by two Connection instances.
    struct Data {
        /// \brief Is the connection still active?
        bool is_connected{ true };
    };

private: // methods for Signal
    /// \brief Value constructor.
    ///
    /// \param data Shared Connection data block.
    Connection(std::shared_ptr<Data> data)
        : m_data(std::move(data))
    {
    }

    /// \brief Creates a new, default constructed Connections object.
    static Connection make_connection()
    {
        return Connection(std::make_shared<Data>());
    }

public: // methods
    Connection(Connection const&) = default;
    Connection& operator=(Connection const&) = default;
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;

    /// \brief Check if the connection is alive.
    ///
    /// \return True if the connection is alive.
    bool is_connected() const { return m_data && m_data->is_connected; }

    /// \brief Breaks this Connection.
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
    /// \brief Data block shared by two Connection instances.
    std::shared_ptr<Data> m_data;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Manager class owned by instances that have methods connected to Signals.
///
/// A Callback instance tracks all Connections representing Signal Targets to a member function of an object,
/// and disconnects them when that object goes out of scope.
/// The Callback member should be placed at the end of the class definition, so it is destructed before any other.
/// This way, all data required for the last remaining calls to finish is still valid.
/// If used within a class hierarchy, the most specialized class has the responsibility to disconnect all of its base
/// class' Signals before destroying any other members.
class Callback {

public: // methods
    /// \brief Default Constructor.
    Callback()
        : m_connections()
    {
    }

    /// \brief Destructor.
    ~Callback() { disconnect_all(); }

    Callback(Callback const&) = delete;
    Callback& operator=(Callback const&) = delete;
    Callback(Callback&&) = delete;
    Callback& operator=(Callback&&) = delete;

    /// \brief Creates and tracks a new Connection between the Signal and target function.
    ///
    /// All arguments after the initial 'Signal' are passed to Signal::connect().
    ///
    /// Connect a method of object 'receiver' like this:
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
        Connection connection = signal.connect(std::forward<ARGS>(args)...);
        m_connections.emplace_back(std::move(connection));
    }

    /// \brief Disconnects all tracked Connections.
    void disconnect_all()
    {
        for (Connection& connection : m_connections) {
            connection.disconnect();
        }
        m_connections.clear();
    }

private: // fields
    /// \brief All managed Connections.
    std::vector<Connection> m_connections;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief An object capable of firing (emitting) signals to connected targets.
template <typename... ARGUMENTS>
class Signal {

    friend class Callback;

private: // struct
    /// \brief Connection and target function pair.
    struct Target {
        Target(Connection connection, std::function<void(ARGUMENTS...)> function,
            std::function<bool(ARGUMENTS...)> test_function = [](ARGUMENTS...) { return true; })
            : connection(std::move(connection))
            , function(std::move(function))
            , test_function(std::move(test_function))
        {
        }

        /// \brief Connection through which the Callback is performed.
        Connection connection;

        /// \brief Callback function.
        std::function<void(ARGUMENTS...)> function;

        /// \brief The signal is only passed over this Connection if this function evaluates to true.
        std::function<bool(ARGUMENTS...)> test_function;
    };

public: // methods
    /// \brief Default constructor.
    Signal()
        : m_targets()
    {
    }

    /// \brief Destructor.
    ~Signal()
    {
        disconnect_all();
    }

    Signal(Signal const&) = delete;
    Signal& operator=(Signal const&) = delete;

    /// \brief Move Constructor.
    ///
    /// \param other
    Signal(Signal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

    /// \brief RValue assignment Operator.
    ///
    /// \param other
    ///
    /// \return This instance.
    Signal& operator=(Signal&& other) noexcept
    {
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
    Connection connect(std::function<void(ARGUMENTS...)> function,
        std::function<bool(ARGUMENTS...)> test_func = {})
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
        } else {
            new_targets.emplace_back(connection, std::move(function));
        }

        // replace the stored targets
        std::swap(m_targets, new_targets);

        return connection;
    }

    /// \brief Disconnect all Connections from this Signal.
    void disconnect_all()
    {
        // disconnect all callbacks
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
    }

    /// \brief Fires (emits) the signal.
    ///
    /// Arguments to this function are passed to the connected callbacks and must match the signature with which the
    /// Signal instance was defined.
    void fire(ARGUMENTS&... args) const
    {
        for (auto& target : m_targets) {
            if (!target.connection.is_connected() || !target.test_function(args...)) {
                continue;
            }
            target.function(args...);
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
    Connection connect(OBJ* obj, void (OBJ::*method)(ARGUMENTS... args), std::function<bool(ARGUMENTS...)> test_func = {})
    {
        assert(obj);
        assert(method);
        return connect([=](ARGUMENTS... args) { (obj->*method)(args...); }, std::forward<std::function<bool(ARGUMENTS...)> >(test_func));
    }

private: // fields
    /// \brief All targets of this Signal.
    std::vector<Target> m_targets;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Full specialization for Signals that require no arguments.
template <>
class Signal<> {

    friend class Callback;

private: // struct
    /// \brief Connection and target function pair.
    struct Target {
        Target(Connection connection, std::function<void()> function)
            : connection(std::move(connection))
            , function(std::move(function))
        {
        }

        /// \brief Connection through which the Callback is performed.
        Connection connection;

        /// \brief Callback function.
        std::function<void()> function;
    };

public: // methods
    /// \brief Default constructor.
    Signal()
        : m_targets()
    {
    }

    /// \brief Destructor.
    ~Signal()
    {
        disconnect_all();
    }

    Signal(Signal const&) = delete;
    Signal& operator=(Signal const&) = delete;

    /// \brief Move Constructor.
    ///
    /// \param other
    Signal(Signal&& other) noexcept
        : m_targets(std::move(other.m_targets))
    {
    }

    /// \brief RValue assignment Operator.
    ///
    /// \param other
    ///
    /// \return This instance.
    Signal& operator=(Signal&& other) noexcept
    {
        m_targets = std::move(other.m_targets);
        return *this;
    }

    /// \brief Connects a new target to this Signal.
    ///
    /// Existing but disconnected Connections are purged before the new target is connected.
    ///
    /// \param function     Target function.
    ///
    /// \return The created Connection.
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

    /// \brief Disconnect all Connections from this Signal.
    void disconnect_all()
    {
        // disconnect all callbacks
        for (auto& target : m_targets) {
            target.connection.disconnect();
        }
    }

    /// \brief Fires (emits) the signal.
    void fire() const
    {
        for (auto& target : m_targets) {
            if (!target.connection.is_connected()) {
                continue;
            }
            target.function();
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
    Connection connect(OBJ* obj, void (OBJ::*method)())
    {
        assert(obj);
        assert(method);
        return connect([=]() { (obj->*method)(); });
    }

private: // fields
    /// \brief All targets of this Signal.
    std::vector<Target> m_targets;
};

} // namespace signal

#if 0
#include <chrono>
#include <iostream>

#include "signal.hpp"
#include "signal_threaded.hpp"
using namespace signal;

#if 0
#define CALLBACK Callback
#define SIGNAL Signal
#else
#define CALLBACK ThreadedCallback
#define SIGNAL ThreadedSignal
#endif


using Clock = std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

/// \brief Free signal receiver.
/// \param value    Value to add
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

/// \brief Free signal receiver for testing values.
/// \param value    Value to print
void freePrintCallback(uint value)
{
    std::cout << "Free print callback: " << value << "\n";
}

/// \brief Simple class with a signal.
class Sender {
public:
    void setValue(uint value) { valueChanged.fire(value); }
    void fireEmpty() { emptySignal.fire(); }
    SIGNAL<uint> valueChanged;
    SIGNAL<> emptySignal;
};

/// \brief Simple receiver class providing two slots.
class Receiver {
public:
    Receiver()
        : counter(0)
    {
    }

    void memberCallback(uint value) { counter += value; }
    void memberPrintCallback(uint value) { std::cout << "valueChanged: " << value << "\n"; }
    ulong counter;
    CALLBACK callbacks; // should be the last member
};

/// \brief Receiver class with an invalid slot.
class BadReceiver {
public:
    void this_shouldnt_work(double) {}
    CALLBACK callbacks;
};

int main()
{
    {
        Receiver* r1 = new Receiver();
        Sender s;
        r1->callbacks.connect(s.valueChanged, r1, &Receiver::memberPrintCallback);
        //        r1->slots.connect(s.valueChanged, r1, &Receiver::valueChanged, [](uint i) { return i == 1; });

        //        Receiver2 double_rec;
        //        double_rec.slots.connect(s.valueChanged, &double_rec, &Receiver2::this_shouldnt_work);
        {
            Receiver r2;
            r2.callbacks.connect(s.valueChanged, &r2, &Receiver::memberPrintCallback);
            s.setValue(15);

            r2.callbacks.disconnect_all();
            s.setValue(13);

            s.valueChanged.connect(freePrintCallback);
        }
        delete r1;
        s.setValue(12);

        std::cout << "Should print 15 / 15 / 13 and free callback 12" << std::endl;
    }

    ulong REPETITIONS = 10000000;

    Sender sender;
    //    sender.counterIncreased.connect(freeCallback);
    sender.emptySignal.connect(emptyCallback);

    Receiver receiver;
    receiver.callbacks.connect(sender.valueChanged, &receiver, &Receiver::memberCallback);

    Clock::time_point t0 = Clock::now();
    for (uint i = 0; i < REPETITIONS; ++i) {
//        sender.setValue(1);
        sender.fireEmpty();
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
