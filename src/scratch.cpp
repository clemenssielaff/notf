#include <iostream>
#include <string>

#include "common/signal.hpp"

using namespace notf;

struct Emitter : public receive_signals {
    Emitter() = default;
    Signal<> signal;
    uint count = 1;
    void slot()
    {
        std::cout << "Emitter " << count++ << std::endl;
        signal();
    }
};

struct Step : public receive_signals {
    Step() = default;
    Signal<> signal;
    uint count = 1;
    void slot()
    {
        std::cout << "Step " << count++ << std::endl;
        signal();
    }
};

struct Closer : public receive_signals {
    Closer() = default;
    Signal<> signal;
    uint count = 1;
    void slot()
    {
        std::cout << "Closer " << count++ << std::endl;
        signal();
    }
};

//int notmain()
int main()
{
    Emitter emitter;
    Step step;
    Closer closer;

    step.connect_signal(emitter.signal, &Step::slot);
    closer.connect_signal(step.signal, &Closer::slot);
    emitter.connect_signal(closer.signal, &Emitter::slot);

    emitter.signal();

    return 0;
}
