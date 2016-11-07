#include "python/pyglobal.hpp"

#include "common/const.hpp"
using namespace notf;

#include "pybind11/pybind11.h"
namespace py = pybind11;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_globals(pybind11::module& module)
{

    py::enum_<STACK_DIRECTION>(module, "STACK_DIRECTION")
        .value("LEFT_TO_RIGHT", STACK_DIRECTION::LEFT_TO_RIGHT)
        .value("TOP_TO_BOTTOM", STACK_DIRECTION::TOP_TO_BOTTOM)
        .value("RIGHT_TO_LEFT", STACK_DIRECTION::RIGHT_TO_LEFT)
        .value("BOTTOM_TO_TOP", STACK_DIRECTION::BOTTOM_TO_TOP);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
