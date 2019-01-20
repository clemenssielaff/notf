#include "catch2/catch.hpp"

#include "notf/meta/time.hpp"

#include "notf/common/mutex.hpp"
#include "notf/common/thread.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("thread", "[common][thread]") {

    SECTION("you can identify a thread by its kind") {
        Thread worker(Thread::Kind::WORKER);
        worker.run([&]() { REQUIRE(this_thread::get_kind() == Thread::Kind::WORKER); });
    }

    SECTION("threads can only run a single function at a time") {
        std::mutex mutex;
        std::condition_variable condition;
        std::atomic_bool is_ready = false;

        Thread worker;
        worker.run([&] {
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [&] { return is_ready.load(); });
            }
        });
        REQUIRE(worker.is_running());
        REQUIRE_THROWS_AS(worker.run([] {}), ThreadError);

        is_ready = true;
        condition.notify_all();
    }

    SECTION("some kinds can only be used once at a time") {
        SECTION("they can be closed and re-opened, as long as there is only one at a time") {
            size_t counter = 0;
            {
                Thread event(Thread::Kind::EVENT);
                event.run([&counter]() { ++counter; });
                event.join();
                REQUIRE(!event.has_exception());
            }
            {
                Thread event(Thread::Kind::EVENT);
                event.run([&counter]() { ++counter; });
                event.join();
                REQUIRE(!event.has_exception());
            }
            REQUIRE(counter == 2);
        }

        SECTION("if you try to have more than one, they will throw") {
            bool has_exception = false;
            std::exception_ptr exception;

            Thread outer_thread(Thread::Kind::EVENT);
            outer_thread.run([&] {
                Thread inner_thread(Thread::Kind::EVENT);
                inner_thread.run([] {});
                inner_thread.join();

                has_exception = inner_thread.has_exception();
                if (has_exception) {
                    try {
                        inner_thread.rethrow();
                    }
                    catch (...) {
                        exception = std::current_exception();
                    }
                }
            });
            outer_thread.join();
            REQUIRE(has_exception);
            auto thrower = [&] { std::rethrow_exception(std::move(exception)); };
            REQUIRE_THROWS_AS(thrower(), ThreadError);
        }
    }

    SECTION("threads can be move-assigned") {
        std::mutex mutex;
        std::condition_variable condition;
        std::atomic_bool is_ready = false;

        Thread original(Thread::Kind::RENDER);
        original.run([&] {
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [&] { return is_ready.load(); });
            }
        });
        REQUIRE(original.is_running());

        Thread other;
        REQUIRE(!other.is_running());
        other = std::move(original);
        REQUIRE(other.is_running());
        REQUIRE(!original.is_running());
        REQUIRE(other.get_kind() == Thread::Kind::RENDER);

        is_ready = true;
        condition.notify_all();
    }

    SECTION("thread ids can be cast to a number") { REQUIRE(to_number(std::this_thread::get_id()) != 0); }
}
