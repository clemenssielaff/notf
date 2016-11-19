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
    PyClaimDirection.def(py::init<float, float, float>(), py::arg("preferred"), py::arg("min") = NAN, py::arg("max") = NAN);

    // inspections
    PyClaimDirection.def("get_preferred", &Claim::Direction::get_preferred, "Preferred size in local units, is >= 0.");
    PyClaimDirection.def("get_min", &Claim::Direction::get_min, "Minimum size in local units, is 0 <= min <= preferred.");
    PyClaimDirection.def("get_max", &Claim::Direction::get_max, "Maximum size in local units, is >= preferred.");
    PyClaimDirection.def("is_fixed", &Claim::Direction::is_fixed, "Tests if this Stretch is a fixed size where all 3 values are the same.");
    PyClaimDirection.def("get_scale_factor", &Claim::Direction::get_scale_factor, "Returns the scale factor of the LayoutItem in this direction.");
    PyClaimDirection.def("get_priority", &Claim::Direction::get_priority, "Returns the scale priority of the LayoutItem in this direction.");

    // modifications
    PyClaimDirection.def("set_preferred", &Claim::Direction::set_preferred, "Sets a new preferred size, accomodates both the min and max size if necessary.", py::arg("preferred"));
    PyClaimDirection.def("set_min", &Claim::Direction::set_min, "Sets a new minimal size, accomodates both the preferred and max size if necessary.", py::arg("min"));
    PyClaimDirection.def("set_max", &Claim::Direction::set_max, "Sets a new maximal size, accomodates both the min and preferred size if necessary.", py::arg("max"));
    PyClaimDirection.def("set_scale_factor", &Claim::Direction::set_scale_factor, "Sets a new scale factor.", py::arg("factor"));
    PyClaimDirection.def("set_priority", &Claim::Direction::set_priority, "Sets a new scaling priority.", py::arg("priority"));
    PyClaimDirection.def("add_offset", &Claim::Direction::add_offset, "Adds an offset to the min, max and preferred value.", py::arg("offset"));

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
            direction.get_preferred(),
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
    PyClaim.def("get_height_for_width", &Claim::get_height_for_width, "Returns the min and max ratio constraints, 0 means no constraint, is: 0 <= min <= max < INFINITY");

    // modifications
    PyClaim.def("set_horizontal", &Claim::set_horizontal, "Sets the horizontal direction of this Claim.", py::arg("direction"));
    PyClaim.def("set_vertical", &Claim::set_vertical, "Sets the vertical direction of this Claim.", py::arg("direction"));
    PyClaim.def("add_horizontal", &Claim::add_horizontal, "In-place, horizontal addition operator for Claims.", py::arg("other"));
    PyClaim.def("add_vertical", &Claim::add_vertical, "In-place, vertical addition operator for Claims.", py::arg("other"));
    PyClaim.def("set_height_for_width", &Claim::set_height_for_width, "Sets the ratio constraint.", py::arg("ratio_min"), py::arg("ratio_max") = NAN);

    // operators
    PyClaim.def(py::self == py::self);
    PyClaim.def(py::self != py::self);

    // representation
    PyClaim.def("__repr__", [](const Claim& claim) {
        const Claim::Direction& horizontal = claim.get_horizontal();
        const Claim::Direction& vertical = claim.get_horizontal();
        const std::pair<float, float> ratio = claim.get_height_for_width();
        return string_format(
            "notf.Claim(\n"
            "\thorizontal: [%f <= %f <=%f, factor: %f, priority %i]\n"
            "\tvertical: [%f <= %f <=%f, factor: %f, priority %i]\n"
            "\tratio: %f : %f)",
            horizontal.get_min(),
            horizontal.get_preferred(),
            horizontal.get_max(),
            horizontal.get_scale_factor(),
            horizontal.get_priority(),
            vertical.get_min(),
            vertical.get_preferred(),
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
