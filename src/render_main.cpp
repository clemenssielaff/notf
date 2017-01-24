#include <iostream>

#include "core/glfw_wrapper.hpp"

void on_error(int error, const char* message)
{
    std::cerr << "GLFW Error " << error << ": '" << message << "'" << std::endl;
}

int main(int /*argc*/, char* /*argv*/[])
{
    // initialize GLFW
    glfwSetErrorCallback(on_error);
    if (!glfwInit()) {
        exit(-1);
    }

    // create the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "NoTF Render Test", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // setup GLFW
    glfwSetTime(0);
    glfwSwapInterval(0);

    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // exit gracefully
    glfwTerminate();
    return 0;
}
