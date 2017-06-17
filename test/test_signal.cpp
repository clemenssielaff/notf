#include "catch.hpp"

#include "common/float.hpp"
#include "common/signal.hpp"
using namespace notf;

namespace { // anonymous

struct Sender {
    Signal<> void_signal;
    Signal<int> int_signal;
    Signal<float> float_signal;
    Signal<bool, bool> two_bool_signal;
};

struct Receiver : public receive_signals {
    void on_void_signal() { ++void_counter; }
    void on_int_signal(int v) { int_counter += v; }
    void on_float_signal(float v) { float_counter += v; }
    void on_two_bool_signal(bool, bool) { ++two_bool_counter; }
    void on_const_signal() const { ++const_counter; }

    int void_counter = 0;
    int int_counter = 0;
    float float_counter = 0.f;
    uint two_bool_counter = 0;
    mutable int const_counter = 0;
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

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

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

    WHEN("you connect a signal to a member function that is const")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect_signal(sender.void_signal, &Receiver::on_const_signal);
        sender.void_signal();

        THEN("it woks as well (should, if compiles)")
        {
            REQUIRE(receiver.const_counter == 1);
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

        receiver.connect_signal(sender.void_signal, [&receiver]() { ++receiver.void_counter; });
        receiver.connect_signal(sender.int_signal, [&receiver](int v) { receiver.int_counter += v; });
        receiver.connect_signal(sender.float_signal, [&receiver](float v) { receiver.float_counter += v; });
        receiver.connect_signal(sender.two_bool_signal, [&receiver](bool, bool) { ++receiver.two_bool_counter; });

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

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

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

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

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

        receiver1.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver1.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver1.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver1.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver2.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver2.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver2.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver2.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

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

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

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

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.disconnect_all_connections();

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

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        sender.void_signal.disconnect();
        sender.int_signal.disconnect();
        sender.float_signal.disconnect();
        sender.two_bool_signal.disconnect();

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

        receiver1.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver1.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver1.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver1.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver2.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver2.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver2.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver2.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver1.disconnect_all_connections();

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
        receiver1.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver1.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver1.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver1.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        {
            Receiver receiver2;
            receiver2.connect_signal(sender.void_signal, &Receiver::on_void_signal);
            receiver2.connect_signal(sender.int_signal, &Receiver::on_int_signal);
            receiver2.connect_signal(sender.float_signal, &Receiver::on_float_signal);
            receiver2.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);
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
        receiver1->connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver1->connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver1->connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver1->connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        {
            auto receiver2 = std::make_unique<Receiver>();
            receiver2->connect_signal(sender.void_signal, &Receiver::on_void_signal);
            receiver2->connect_signal(sender.int_signal, &Receiver::on_int_signal);
            receiver2->connect_signal(sender.float_signal, &Receiver::on_float_signal);
            receiver2->connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);
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
            receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
            receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
            receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
            receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

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

        receiver.connect_signal(sender1.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender1.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender1.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender1.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.connect_signal(sender2.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender2.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender2.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender2.two_bool_signal, &Receiver::on_two_bool_signal);

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

    WHEN("you connect a signal to a member function with a test function")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal, [](int v) { return v == 1; });
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal, [](float v) { return v == 1.f; });
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal, [](bool a, bool b) { return a == b; });

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

    WHEN("you connect a signal to a function and dis/enable the signal")
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

        sender.void_signal.disable();
        sender.int_signal.disable();
        sender.float_signal.disable();
        sender.two_bool_signal.disable();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("the function is not called when the signal is fired while disabled")
        {
            REQUIRE(free_void_counter == 0);
            REQUIRE(free_int_counter == 0);
            REQUIRE(free_float_counter == approx(0, 0));
            REQUIRE(free_two_bool_counter == 0);
        }

        sender.void_signal.enable();
        sender.int_signal.enable();
        sender.float_signal.enable();
        sender.two_bool_signal.enable();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("but is called when the signal is fired while enabled again")
        {
            REQUIRE(free_void_counter == 1);
            REQUIRE(free_int_counter == 123);
            REQUIRE(free_float_counter == approx(1.23f));
            REQUIRE(free_two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to an object and dis/enable its slots")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_float_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_two_bool_signal);

        receiver.disable_all_connections();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("the slots are not called when disabled")
        {
            REQUIRE(receiver.void_counter == 0);
            REQUIRE(receiver.int_counter == 0);
            REQUIRE(receiver.float_counter == approx(0, 0));
            REQUIRE(receiver.two_bool_counter == 0);
        }

        receiver.enable_all_connections();

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("but called again when enabled")
        {
            REQUIRE(receiver.void_counter == 1);
            REQUIRE(receiver.int_counter == 123);
            REQUIRE(receiver.float_counter == approx(1.23f));
            REQUIRE(receiver.two_bool_counter == 1);
        }
    }

    WHEN("you connect a signal to a method without arguments")
    {
        Sender sender;
        Receiver receiver;

        receiver.connect_signal(sender.void_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.int_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.float_signal, &Receiver::on_void_signal);
        receiver.connect_signal(sender.two_bool_signal, &Receiver::on_void_signal);

        sender.void_signal();
        sender.int_signal(123);
        sender.float_signal(1.23f);
        sender.two_bool_signal(true, true);

        THEN("the slot is called as if the signals' signatures matched")
        {
            REQUIRE(receiver.void_counter == 4);
        }
    }

    WHEN("you disable and enable specific connections from a signal")
    {
        Sender sender;
        Receiver receiver;

        Connection a = receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal, [](int i) { return i == 3; });
        Connection b = receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal, [](int i) { return i == 5; });

        THEN("only the enabled connections are used")
        {
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 8);

            a.disable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 13);

            b.disable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 13);

            a.enable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 16);

            b.enable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 24);
        }
    }

    WHEN("you disable and enable specific connections from the receiver")
    {
        Sender sender;
        Receiver receiver;

        Connection a = receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal, [](int i) { return i == 3; });
        Connection b = receiver.connect_signal(sender.int_signal, &Receiver::on_int_signal, [](int i) { return i == 5; });

        THEN("only the enabled connections are used")
        {
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 8);

            a.disable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 13);

            b.disable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 13);

            a.enable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 16);

            b.enable();
            sender.int_signal(3);
            sender.int_signal(4);
            sender.int_signal(5);
            REQUIRE(receiver.int_counter == 24);
        }
    }
}
