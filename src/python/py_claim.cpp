#include "pybind11/operators.h"
#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "common/claim.hpp"
#include "common/string_utils.hpp"
using namespace notf;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

void produce_claim(pybind11::module& module)
{

    /* Claim::Direction ***********************************************************************************************/

    py::class_<Claim::Direction> PyClaimDirection(module, "ClaimDirection");

    // constructors
    PyClaimDirection.def(py::init<>());
    PyClaimDirection.def(py::init<float, float, float>(), py::arg("base"), py::arg("min") = NAN, py::arg("max") = NAN);

    // inspections
    PyClaimDirection.def("get_base", &Claim::Direction::get_base, "Base size in local units, is `0 <= base`.");
    PyClaimDirection.def("get_min", &Claim::Direction::get_min, "Minimum size in local units, is `0 <= min <= max`.");
    PyClaimDirection.def("get_max", &Claim::Direction::get_max, "Maximum size in local units, is `min <= max`.");
    PyClaimDirection.def("is_fixed", &Claim::Direction::is_fixed, "Tests if this Direction is fixed, where both `min` and `max` are the same.");
    PyClaimDirection.def("get_scale_factor", &Claim::Direction::get_scale_factor, "Returns the scale factor of the LayoutItem in this direction.");
    PyClaimDirection.def("get_priority", &Claim::Direction::get_priority, "Returns the scale priority of the LayoutItem in this direction.");

    // modifications
    PyClaimDirection.def("set_base", &Claim::Direction::set_base, "Sets a new base size, does not interact with `min` or `max`, is `0 <= base`.", py::arg("base"));
    PyClaimDirection.def("set_min", &Claim::Direction::set_min, "Sets a new minimal size, accomodates `max` if necessary.", py::arg("min"));
    PyClaimDirection.def("set_max", &Claim::Direction::set_max, "Sets a new maximal size, accomodates `min` if necessary.", py::arg("max"));
    PyClaimDirection.def("set_scale_factor", &Claim::Direction::set_scale_factor, "Sets a new scale factor.", py::arg("factor"));
    PyClaimDirection.def("set_priority", &Claim::Direction::set_priority, "Sets a new scaling priority.", py::arg("priority"));
    PyClaimDirection.def("add_offset", &Claim::Direction::add_offset, "Adds an offset to the min, max and base value.", py::arg("offset"));

    // operators
    PyClaimDirection.def(py::self == py::self);
    PyClaimDirection.def(py::self != py::self);
    PyClaimDirection.def(py::self += py::self);

    PyClaimDirection.def("maxed", &Claim::Direction::maxed, "In-place max operator for Directions.", py::arg("other"));

    // representation
    PyClaimDirection.def("__repr__", [](const Claim::Direction& direction) {
        return string_format(
            "notf.Claim::Direction([%f <= %f <=%f, factor: %f, priority %i])",
            direction.get_min(),
            direction.get_base(),
            direction.get_max(),
            direction.get_scale_factor(),
            direction.get_priority());
    });

    /* Claim **********************************************************************************************************/

    py::class_<Claim> PyClaim(module, "Claim");

    // constructors
    PyClaim.def(py::init<>());

    // inspections
    PyClaim.def("get_horizontal", (Claim::Direction & (Claim::*)()) & Claim::get_horizontal, "Returns the horizontal part of this Claim.");
    PyClaim.def("get_vertical", (Claim::Direction & (Claim::*)()) & Claim::get_vertical, "Returns the vertical part of this Claim.");
    PyClaim.def("get_width_to_height", &Claim::get_width_to_height, "Returns the min and max ratio constraints, 0 means no constraint, is: 0 <= min <= max < INFINITY");

    // modifications
    PyClaim.def("set_horizontal", &Claim::set_horizontal, "Sets the horizontal direction of this Claim.", py::arg("direction"));
    PyClaim.def("set_vertical", &Claim::set_vertical, "Sets the vertical direction of this Claim.", py::arg("direction"));
    PyClaim.def("add_horizontal", &Claim::add_horizontal, "In-place, horizontal addition operator for Claims.", py::arg("other"));
    PyClaim.def("add_vertical", &Claim::add_vertical, "In-place, vertical addition operator for Claims.", py::arg("other"));
    PyClaim.def("set_width_to_height", &Claim::set_width_to_height, "Sets the ratio constraint.", py::arg("ratio_min"), py::arg("ratio_max") = NAN);

    // operators
    PyClaim.def(py::self == py::self);
    PyClaim.def(py::self != py::self);

    // representation
    PyClaim.def("__repr__", [](const Claim& claim) {
        const Claim::Direction& horizontal = claim.get_horizontal();
        const Claim::Direction& vertical = claim.get_horizontal();
        const std::pair<float, float> ratio = claim.get_width_to_height();
        return string_format(
            "notf.Claim(\n"
            "\thorizontal: [base: %f, limits: %f <=%f, factor: %f, priority %i]\n"
            "\tvertical: [base: %f, limits: %f <=%f, factor: %f, priority %i]\n"
            "\tratio: %f : %f)",
            horizontal.get_base(),
            horizontal.get_min(),
            horizontal.get_max(),
            horizontal.get_scale_factor(),
            horizontal.get_priority(),
            vertical.get_base(),
            vertical.get_min(),
            vertical.get_max(),
            vertical.get_scale_factor(),
            vertical.get_priority(),
            ratio.first,
            ratio.second);
    });
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
