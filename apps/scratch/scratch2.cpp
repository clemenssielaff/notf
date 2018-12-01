#include "notf/common/timer_pool.hpp"
#include "notf/common/random.hpp"

int main() {
    NOTF_USING_NAMESPACE;
    NOTF_USING_LITERALS;

    std::cout << "starting" << std::endl;
    {
        TimerPool pool;

        auto random_timer = VariableTimer([] { std::cout << "so random" << std::endl; },
                                          [] { return duration_t(to_seconds(random(0.1, 2.0))); },
                                          /* repetitions = */ 5);
        random_timer->set_anonymous();
        random_timer->set_keep_alive();
        pool.schedule(random_timer);
        //        auto interval = IntervalTimer(0.8s, [] { std::cout << "interval derbness" << std::endl; });
        //        auto copy = interval;
        //        pool.schedule(std::move(interval));
        //        //        pool.schedule(IntervalTimer(0.8s, [] { std::cout << "interval derbness" << std::endl; }));
        //        pool.schedule(OneShotTimer(now() + 1s, [] { std::cout << "derbe after 1 seconds" << std::endl; }));
        //        pool.schedule(OneShotTimer(now() + 2s, [] { std::cout << "derbe after 2 seconds" << std::endl; }));
        //        pool.schedule(OneShotTimer(now() + 3s, [] { std::cout << "derbe after 3 seconds" << std::endl; }));
        //        std::this_thread::sleep_for(5s);
    }
    std::cout << "closing" << std::endl;
    return 0;
}
