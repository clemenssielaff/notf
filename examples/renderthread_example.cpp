#include "renderthread_example.hpp"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

#include "app/core/glfw.hpp"
#include "common/log.hpp"

NOTF_USING_NAMESPACE

using namespace std::chrono_literals;

namespace {

std::mutex render_mutex;
std::condition_variable render_condition_variable;
bool next_frame_ready = false;

void render_thread(GLFWwindow* window)
{
    size_t count = 1;
    while (1) {

        { // Wait until main() sends data
            std::unique_lock<std::mutex> lock_guard(render_mutex);
            if (!next_frame_ready) {
                render_condition_variable.wait(lock_guard, [] { return next_frame_ready; });
            }
            next_frame_ready = false;
        }

        std::cout << "Refresh:\t" << count++ << std::endl;

        if (glfwWindowShouldClose(window)) {
            return;
        }

        std::this_thread::sleep_for(0.1s);
    }
}

static void error_callback(int error, const char* description)
{
    log_critical << "GLFW error #" << error << ": " << description;
}

} // namespace

int renderthread_main(int /*argc*/, char* /*argv*/ [])
{
    // install the log handler first, to catch errors right away
    auto log_handler = std::make_unique<LogHandler>(128, 200);
    install_log_message_handler(std::bind(&LogHandler::push_log, log_handler.get(), std::placeholders::_1));
    log_handler->start();
    glfwSetErrorCallback(error_callback);

    // initialize GLFW
    if (!glfwInit()) {
        log_fatal << "GLFW initialization failed";
        exit(-1);
    }
    log_info << "GLFW version: " << glfwGetVersionString();

    // NoTF uses OpenGL ES 3.2
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    { // open the window
        GLFWwindow* window = glfwCreateWindow(800, 800, "NoTF Engine Test", nullptr, nullptr);
        std::thread render_worker(render_thread, window);
        size_t spinner = 0;
        while (!glfwWindowShouldClose(window)) {

            glfwWaitEvents();

            // send data to the worker thread
            {
                std::lock_guard<std::mutex> lock_guard(render_mutex);
                next_frame_ready = true;
            }
            render_condition_variable.notify_one();

            ++spinner;
        }

        render_worker.join();
        glfwDestroyWindow(window);

        std::cout << "Spinner on main: " << spinner << std::endl;
    }

    // stop the event loop
    glfwTerminate();

    // stop the logger
    log_info << "Application shutdown";
    log_handler->stop();
    log_handler->join();

    return 0;
}
