#include "test/catch.hpp"

#include "common/float_utils.hpp"
#include "common/signal.hpp"
using notf::Signal;
using notf::CallbackManager;
using notf::approx;

namespace { // anonymous

struct Sender {
    Signal<> void_signal;
    Signal<int> int_signal;
    Signal<float> float_signal;
    Signal<bool, bool> two_bool_signal;
};

struct Receiver {
    template <typename SIGNAL, typename... ARGS>
    void connect(SIGNAL& signal, ARGS&&... args) { m_callbacks.connect(signal, std::forward<ARGS>(args)...); }

    template <typename SIGNAL, typename... SIGNATURE, typename... TEST_FUNC>
    void connect(SIGNAL& signal, void (Receiver::*method)(SIGNATURE...), TEST_FUNC&&... test_func)
    {
        m_callbacks.connect(signal, this, std::forward<decltype(method)>(method), std::forward<TEST_FUNC>(test_func)...);
    }

    void disconnect_all() { m_callbacks.disconnect_all(); }

    void on_void_signal() { ++void_counter; }
    void on_int_signal(int v) { int_counter += v; }
    void on_float_signal(float v) { float_counter += v; }
    void on_two_bool_signal(bool, bool) { ++two_bool_counter; }

    int void_counter = 0;
    int int_counter = 0;
    float float_counter = 0.f;
    uint two_bool_counter = 0;

private:
    CallbackManager m_callbacks;
};

int free_void_counter = 0;
int free_int_counter = 0;
float free_float_counter = 0;
uint free_two_bool_counter = 0;

void free_void_function() { ++free_void_counter; }
void free_int_function(int v) { free_int_counter += v; }
void free_float_function(float v) { free_float_counter += v; }
void free_two_bool_function(bool, bool) { ++free_two_bool_counter; }

} // namespace anonymous

SCENARIO("signals are dynamic callbacks between functions and methods", "[common][signal]")
{
    WHEN("a signal is fired without a connected callback")
    {
        Sender sender;

        sender.void_signal();
        sender.int_signal(1);
        sender.float_signal(1.);
        sender.two_bool_signal(true, false);

        THEN("nothing happens")
        {
        }
    }

    WHEN("you connect a signal to a member function")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("it is is executed exactly once")
        {
            REQUIRE(receiver.void_counter == 1);
            REQUIRE(receiver.int_counter == 123);
            REQUIRE(receiver.float_counter == approx(1.23f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to a free function")
    {
        Sender sender;

        free_void_counter = 0;
        free_int_counter = 0;
        free_float_counter = 0.f;
        free_two_bool_counter = 0;

        sender.void_signal.connect(free_void_function);
        sender.int_signal.connect(free_int_function);
        sender.float_signal.connect(free_float_function);
        sender.two_bool_signal.connect(free_two_bool_function);

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("it is called just like any other callback")
        {
            REQUIRE(free_void_counter == 1);
            REQUIRE(free_int_counter == 123);
            REQUIRE(free_float_counter == approx(1.23f));
            REQUIRE(free_two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to a lambda function")
    {
        Sender sender;

        int lambda_void_counter = 0;
        int lambda_int_counter = 0;
        float lambda_float_counter = 0;
        uint lambda_two_bool_counter = 0;

        sender.void_signal.connect([&]() { ++lambda_void_counter; });
        sender.int_signal.connect([&](int v) { lambda_int_counter += v; });
        sender.float_signal.connect([&](float v) { lambda_float_counter += v; });
        sender.two_bool_signal.connect([&](bool, bool) { ++lambda_two_bool_counter; });

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("it is called just like any other callback")
        {
            REQUIRE(lambda_void_counter == 1);
            REQUIRE(lambda_int_counter == 123);
            REQUIRE(lambda_float_counter == approx(1.23f));
            REQUIRE(lambda_two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to a lambda function that is managed by a receiver")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.void_signal, [&receiver]() { ++receiver.void_counter; });
        receiver.connect(sender.int_signal, [&receiver](int v) { receiver.int_counter += v; });
        receiver.connect(sender.float_signal, [&receiver](float v) { receiver.float_counter += v; });
        receiver.connect(sender.two_bool_signal, [&receiver](bool, bool) { ++receiver.two_bool_counter; });

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("it is called just like any other callback")
        {
            REQUIRE(receiver.void_counter == 1);
            REQUIRE(receiver.int_counter == 123);
            REQUIRE(receiver.float_counter == approx(1.23f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("you emit signal with an l-value")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        int int_value = 123;
        float float_value = 1.23f;
        bool bool_value = true;

        sender.void_signal();
        sender.int_signal(int_value);
        sender.float_signal(float_value);
        sender.two_bool_signal(bool_value, bool_value);

        THEN("it works just as with an r-value")
        {
            REQUIRE(receiver.void_counter == 1);
            REQUIRE(receiver.int_counter == 123);
            REQUIRE(receiver.float_counter == approx(1.23f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("a signal is fired with a single callback connected twice")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("the callback is executed twice")
        {
            REQUIRE(receiver.void_counter == 2);
            REQUIRE(receiver.int_counter == 246);
            REQUIRE(receiver.float_counter == approx(2.46f));
            REQUIRE(receiver.two_bool_counter == 2);
        }
    }

    WHEN("a signal is fired with a two callbacks connected")
    {
        Sender sender;
        Receiver receiver1;
        Receiver receiver2;

        receiver1.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver1.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver1.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver1.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver2.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver2.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver2.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver2.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("each callback is executed once")
        {
            REQUIRE(receiver1.void_counter == 1);
            REQUIRE(receiver1.int_counter == 123);
            REQUIRE(receiver1.float_counter == approx(1.23f));
            REQUIRE(receiver1.two_bool_counter == 1);

            REQUIRE(receiver2.void_counter == 1);
            REQUIRE(receiver2.int_counter == 123);
            REQUIRE(receiver2.float_counter == approx(1.23f));
            REQUIRE(receiver2.two_bool_counter == 1);
        }
    }

    WHEN("a singal is fired before it is connected and once afterwards")
    {
        Sender sender;
        Receiver receiver;

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("the callback is only called the second time")
        {
            REQUIRE(receiver.void_counter == 1);
            REQUIRE(receiver.int_counter == 123);
            REQUIRE(receiver.float_counter == approx(1.23f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("a callback is connected and disconnected again")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.disconnect_all();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("firing the signal will not call the callback")
        {
            REQUIRE(receiver.void_counter == 0);
            REQUIRE(receiver.int_counter == 0);
            REQUIRE(receiver.float_counter == approx(0.f));
            REQUIRE(receiver.two_bool_counter == 0);
        }
    }

    WHEN("a signal is disconnected")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        sender.void_signal.disconnect_all();
        sender.int_signal.disconnect_all();
        sender.float_signal.disconnect_all();
        sender.two_bool_signal.disconnect_all();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("previously connected callbacks will not be executed")
        {
            REQUIRE(receiver.void_counter == 0);
            REQUIRE(receiver.int_counter == 0);
            REQUIRE(receiver.float_counter == approx(0.f));
            REQUIRE(receiver.two_bool_counter == 0);
        }
    }

    WHEN("you disconnect a callback while other ones are still connected")
    {
        Sender sender;
        Receiver receiver1;
        Receiver receiver2;

        receiver1.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver1.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver1.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver1.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver2.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver2.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver2.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver2.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver1.disconnect_all();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("it doesn't influence the connected callbacks")
        {
            REQUIRE(receiver1.void_counter == 0);
            REQUIRE(receiver1.int_counter == 0);
            REQUIRE(receiver1.float_counter == approx(0.f));
            REQUIRE(receiver1.two_bool_counter == 0);

            REQUIRE(receiver2.void_counter == 1);
            REQUIRE(receiver2.int_counter == 123);
            REQUIRE(receiver2.float_counter == approx(1.23f));
            REQUIRE(receiver2.two_bool_counter == 1);
        }
    }

    WHEN("a receiver on the stack goes out of scope")
    {
        Sender sender;

        Receiver receiver1;
        receiver1.connect(sender.void_signal, &Receiver::on_void_signal);
        receiver1.connect(sender.int_signal, &Receiver::on_int_signal);
        receiver1.connect(sender.float_signal, &Receiver::on_float_signal);
        receiver1.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        {
            Receiver receiver2;
            receiver2.connect(sender.void_signal, &Receiver::on_void_signal);
            receiver2.connect(sender.int_signal, &Receiver::on_int_signal);
            receiver2.connect(sender.float_signal, &Receiver::on_float_signal);
            receiver2.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);
        }

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("everything is as if the receiver was disconnected first")
        {
            REQUIRE(receiver1.void_counter == 1);
            REQUIRE(receiver1.int_counter == 123);
            REQUIRE(receiver1.float_counter == approx(1.23f));
            REQUIRE(receiver1.two_bool_counter == 1);
        }
    }

    WHEN("a receiver on the heap goes out of scope")
    {
        Sender sender;

        auto receiver1 = std::make_unique<Receiver>();
        receiver1->connect(sender.void_signal, &Receiver::on_void_signal);
        receiver1->connect(sender.int_signal, &Receiver::on_int_signal);
        receiver1->connect(sender.float_signal, &Receiver::on_float_signal);
        receiver1->connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        {
            auto receiver2 = std::make_unique<Receiver>();
            receiver2->connect(sender.void_signal, &Receiver::on_void_signal);
            receiver2->connect(sender.int_signal, &Receiver::on_int_signal);
            receiver2->connect(sender.float_signal, &Receiver::on_float_signal);
            receiver2->connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);
        }

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("everything is as if the receiver was disconnected first")
        {
            REQUIRE(receiver1->void_counter == 1);
            REQUIRE(receiver1->int_counter == 123);
            REQUIRE(receiver1->float_counter == approx(1.23f));
            REQUIRE(receiver1->two_bool_counter == 1);
        }
    }

    WHEN("a signal goes out of scope")
    {
        Receiver receiver;
        {
            Sender sender;
            receiver.connect(sender.void_signal, &Receiver::on_void_signal);
            receiver.connect(sender.int_signal, &Receiver::on_int_signal);
            receiver.connect(sender.float_signal, &Receiver::on_float_signal);
            receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal);

            sender.void_signal();
            sender.int_signal(123);
            sender.float_signal(1.23f);
            sender.two_bool_signal(true, true);
        }

        THEN("connected Callbacks don't care")
        {
            REQUIRE(receiver.void_counter == 1);
            REQUIRE(receiver.int_counter == 123);
            REQUIRE(receiver.float_counter == approx(1.23f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("the same callback is be connected to two signals")
    {
        Sender sender1;
        Sender sender2;
        Receiver receiver;

        receiver.connect(sender1.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender1.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender1.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender1.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.connect(sender2.void_signal, &Receiver::on_void_signal);
        receiver.connect(sender2.int_signal, &Receiver::on_int_signal);
        receiver.connect(sender2.float_signal, &Receiver::on_float_signal);
        receiver.connect(sender2.two_bool_signal, &Receiver::on_two_bool_signal);

        sender1.void_signal();
        sender1.int_signal(123);
        sender1.float_signal(1.23f);
        sender1.two_bool_signal(true, true);

        sender2.void_signal();
        sender2.int_signal(123);
        sender2.float_signal(1.23f);
        sender2.two_bool_signal(true, true);

        THEN("they are simply called once for each signal")
        {
            REQUIRE(receiver.void_counter == 2);
            REQUIRE(receiver.int_counter == 246);
            REQUIRE(receiver.float_counter == approx(2.46f));
            REQUIRE(receiver.two_bool_counter == 2);
        }
    }

    WHEN("you try to connect incompatible signals")
    {
        // the compiler complains ... what we can't really test for
        // if in doubt, try to uncomment the next few lines and if it works, something has gone wrong
        Sender sender;
        Receiver receiver;

        //        receiver.connect(sender.void_signal, &Receiver::on_int_signal);
        //        receiver.connect(sender.int_signal, &Receiver::on_float_signal);
        //        receiver.connect(sender.float_signal, &Receiver::on_two_bool_signal);
        //        receiver.connect(sender.two_bool_signal, &Receiver::on_void_signal);
    }

    WHEN("you connect a signal to a member function with a test function")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect(sender.int_signal, &Receiver::on_int_signal, [](int v) { return v == 1; });
        receiver.connect(sender.float_signal, &Receiver::on_float_signal, [](float v) { return v == 1.f; });
        receiver.connect(sender.two_bool_signal, &Receiver::on_two_bool_signal, [](bool a, bool b) { return a == b; });

        sender.int_signal(1);
        sender.int_signal(123);
        sender.float_signal(1.f);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);
        sender.two_bool_signal(false, true);

        THEN("the callback is only executed when the test function returns true")
        {
            REQUIRE(receiver.void_counter == 0);
            REQUIRE(receiver.int_counter == 1);
            REQUIRE(receiver.float_counter == approx(1.0f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to a free function with a test function")
    {
        Sender sender;

        free_int_counter = 0;
        free_float_counter = 0.f;
        free_two_bool_counter = 0;

        sender.int_signal.connect(free_int_function, [](int v) { return v == 1; });
        sender.float_signal.connect(free_float_function, [](float v) { return v == 1.f; });
        sender.two_bool_signal.connect(free_two_bool_function, [](bool a, bool b) { return a == b; });

        sender.int_signal(1);
        sender.int_signal(123);
        sender.float_signal(1.f);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);
        sender.two_bool_signal(false, true);

        THEN("it is called just like any other callback")
        {
            REQUIRE(free_int_counter == 1);
            REQUIRE(free_float_counter == approx(1.f));
            REQUIRE(free_two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to a lambda function with a test function")
    {
        Sender sender;

        int lambda_int_counter = 0;
        float lambda_float_counter = 0;
        uint lambda_two_bool_counter = 0;

        sender.int_signal.connect([&](int v) { lambda_int_counter += v; }, [](int v) { return v == 1; });
        sender.float_signal.connect([&](float v) { lambda_float_counter += v; }, [](float v) { return v == 1.f; });
        sender.two_bool_signal.connect([&](bool a, bool b) { if(a && b) ++lambda_two_bool_counter; }, [](bool a, bool b) { return a == b; });

        sender.int_signal(1);
        sender.int_signal(123);
        sender.float_signal(1.f);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);
        sender.two_bool_signal(false, true);

        THEN("it is called just like any other callback")
        {
            REQUIRE(lambda_int_counter == 1);
            REQUIRE(lambda_float_counter == approx(1.f));
            REQUIRE(lambda_two_bool_counter == 1);
        }
    }
}
