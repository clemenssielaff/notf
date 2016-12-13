#include <chrono>
#include <iostream>
#include "common/signal.hpp"

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
class Receiver : public receive_signals<Receiver>{
public:
    void memberCallback(uint value) { counter += value; }
    void memberPrintCallback(uint value) { std::cout << "valueChanged: " << value << "\n"; }
    ulong counter = 0;
};

int notmain()
//int main()
{
    ulong REPETITIONS = 10000000;
    Sender sender;
    //    sender.counterIncreased.connect(freeCallback);
    sender.emptySignal.connect(emptyCallback);
    Receiver receiver;
    receiver.connect_signal(sender.valueChanged, &Receiver::memberCallback);
    Clock::time_point t0 = Clock::now();
    for (uint i = 0; i < REPETITIONS; ++i) {
        sender.setValue(1);
//        sender.fireEmpty();
//        emptyCallback();
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
