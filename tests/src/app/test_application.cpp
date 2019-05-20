#include "catch.hpp"

#include "notf/common/thread.hpp"

#include "notf/app/application.hpp"
#include "notf/app/graph/window.hpp"

#include "test/app.hpp"

NOTF_USING_NAMESPACE;

SCENARIO("TheApplication Singleton", "[app][application]") {

    SECTION("Requesting TheApplication without a running instance will throw `SingletonError`") {
        REQUIRE_THROWS_AS(TheApplication()->get_arguments(), SingletonError);
    }

    SECTION("a valid Application singleton") {
        TheApplication::Arguments args = test_app_arguments();
        args.start_without_windows = true;
        TheApplication app(args);

        SECTION("can be accessed anywhere in the code") { REQUIRE(TheApplication()->get_arguments().argc == -1); }

        SECTION("can be started (with exec) and shut down") {
            TheApplication()->schedule([] { TheApplication()->shutdown(); }); // shutdown as soon as you start
            int result = TheApplication()->exec();
            REQUIRE(result == EXIT_SUCCESS);
            REQUIRE_THROWS_AS(TheApplication()->exec(), TheApplication::StartupError);
        }

        SECTION("can run exec only from the main thread") {
            Thread other;
            other.run([] { TheApplication()->exec(); });
            other.join();
            REQUIRE(other.has_exception());
            REQUIRE_THROWS_AS(other.rethrow(), TheApplication::StartupError);
        }

        SECTION("you can test whether this is the current UI thread from anywhere") {
            REQUIRE(this_thread::is_the_ui_thread());
            bool hopefully_not = true;
            Thread().run([&] { hopefully_not = this_thread::is_the_ui_thread(); });
            REQUIRE(!hopefully_not);
        }

        SECTION("you can schedule functions to run on the main thread") {
            size_t counter = 0;
            Thread other_thread;
            other_thread.run([&counter] {
                std::mutex mutex;
                std::condition_variable variable;
                bool is_ready = false;

                TheApplication()->schedule([&mutex, &variable, &is_ready, &counter]() {
                    {
                        counter = 9001;
                        NOTF_GUARD(std::lock_guard(mutex));
                        is_ready = true;
                    }
                    variable.notify_one();
                });

                {
                    auto lock = std::unique_lock(mutex);
                    variable.wait(lock, [&is_ready] { return is_ready; });
                    REQUIRE(counter == 9001);
                }
                TheApplication()->shutdown();
            });
            TheApplication()->exec();
        }
    }
}
