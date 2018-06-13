#pragma once

#include "app/forwards.hpp"

NOTF_OPEN_NAMESPACE

/// The global Window to be used in tests.
/// Is defined in main.cpp.
WindowPtr notf_window();

namespace test {

/// Access struct used for various access types.
struct Test {};

} // namespace test

NOTF_CLOSE_NAMESPACE
