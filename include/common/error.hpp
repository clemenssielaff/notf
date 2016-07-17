#pragma once

#include <string>

namespace untitled {

///
/// \brief Codes for all (conveivable) errors that the Application may produce.
///
enum ERROR_CODE {
    NO_ERROR = 0,
    GLFW_INITIALIZATION_FAILURE,
    WINDOW_CONTEXT_CREATION_FAILURE,
    CALLBACK_FOR_UNKNOWN_GLFW_WINDOW,

    // GLFW errors
    GLFW_NOT_INITIALIZED = 65537,
    GLFW_NO_CURRENT_CONTEXT,
    GLFW_INVALID_ENUM,
    GLFW_INVALID_VALUE,
    GLFW_OUT_OF_MEMORY,
    GLFW_API_UNAVAILABLE,
    GLFW_VERSION_UNAVAILABLE,
    GLFW_PLATFORM_ERROR,
    GLFW_FORMAT_UNAVAILABLE,
    GLFW_NO_WINDOW_CONTEXT,
};

struct Error {
    ///
    /// \brief Error ID.
    ///
    int id;

    ///
    /// \brief Error message.
    ///
    std::string message;
};

} // namespace untitled
