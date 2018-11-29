#include "catch2/catch.hpp"

#include "notf/meta/time.hpp"

#include "notf/common/thread.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("thread", "[common][thread]") {
    SECTION("you can identify a thread by its kind") {
        Thread worker(Thread::Kind::WORKER);
        worker.run([&]() { REQUIRE(this_thread::get_kind() == Thread::Kind::WORKER); });
    }

    SECTION("some kinds can only be used once at a time") {
//        SECTION("they can be closed and re-opened, as long as there is only one at a time") {
//            size_t counter = 0;
//            {
//                Thread event(Thread::Kind::EVENT);
//                event.run([&counter]() { ++counter; });
//                event.join();
//                REQUIRE(!event.has_exception());
//            }
//            {
//                Thread event(Thread::Kind::EVENT);
//                event.run([&counter]() { ++counter; });
//                event.join();
//                REQUIRE(!event.has_exception());
//            }
//            REQUIRE(counter == 2);
//        }

//        SECTION("if you try to have more than one, they will throw") {
//            using namespace notf::literals;
//            Thread event1(Thread::Kind::EVENT);
//            Thread event2(Thread::Kind::EVENT);
//            event1.run([]() { this_thread::sleep_for(100ns); });
//            event2.run([]() { this_thread::sleep_for(100ns); });
//            event1.join();
//            event2.join();
//            bool exception_caught = event1.has_exception() || event2.has_exception();
//            REQUIRE(exception_caught);
//        }
    }
}
