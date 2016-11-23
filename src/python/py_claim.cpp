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

    /* Claim::Stretch ***********************************************************************************************/

    py::class_<Claim::Stretch> PyClaimStretch(module, "ClaimStretch");

    // constructors
    PyClaimStretch.def(py::init<>());
    PyClaimStretch.def(py::init<float, float, float>(), py::arg("preferred"), py::arg("min") = NAN, py::arg("max") = NAN);

    // inspections
    PyClaimStretch.def("get_preferred", &Claim::Stretch::get_preferred, "Preferred size in local units, is >= 0.");
    PyClaimStretch.def("get_min", &Claim::Stretch::get_min, "Minimum size in local units, is 0 <= min <= preferred.");
    PyClaimStretch.def("get_max", &Claim::Stretch::get_max, "Maximum size in local units, is >= preferred.");
    PyClaimStretch.def("is_fixed", &Claim::Stretch::is_fixed, "Tests if this Stretch is a fixed size where all 3 values are the same.");
    PyClaimStretch.def("get_scale_factor", &Claim::Stretch::get_scale_factor, "Returns the scale factor.");
    PyClaimStretch.def("get_priority", &Claim::Stretch::get_priority, "Returns the scale priority.");

    // modifications
    PyClaimStretch.def("set_preferred", &Claim::Stretch::set_preferred, "Sets a new preferred size, accomodates both the min and max size if necessary.", py::arg("preferred"));
    PyClaimStretch.def("set_min", &Claim::Stretch::set_min, "Sets a new minimal size, accomodates both the preferred and max size if necessary.", py::arg("min"));
    PyClaimStretch.def("set_max", &Claim::Stretch::set_max, "Sets a new maximal size, accomodates both the min and preferred size if necessary.", py::arg("max"));
    PyClaimStretch.def("set_scale_factor", &Claim::Stretch::set_scale_factor, "Sets a new scale factor.", py::arg("factor"));
    PyClaimStretch.def("set_priority", &Claim::Stretch::set_priority, "Sets a new scaling priority.", py::arg("priority"));
    PyClaimStretch.def("set_fixed", &Claim::Stretch::set_fixed, "Sets a fixed size.", py::arg("size"));
    PyClaimStretch.def("add_offset", &Claim::Stretch::add_offset, "Adds an offset to the min, max and preferred value.", py::arg("offset"));

    // operators
    PyClaimStretch.def(py::self == py::self);
    PyClaimStretch.def(py::self != py::self);
    PyClaimStretch.def(py::self += py::self);

    PyClaimStretch.def("maxed", &Claim::Stretch::maxed, "In-place max operator.", py::arg("other"));

    // representation
    PyClaimStretch.def("__repr__", [](const Claim::Stretch& stretch) {
        return string_format(
            "notf.Claim::Stretch([%f <= %f <=%f, factor: %f, priority %i])",
            stretch.get_min(),
            stretch.get_preferred(),
            stretch.get_max(),
            stretch.get_scale_factor(),
            stretch.get_priority());
    });

    /* Claim **********************************************************************************************************/

    py::class_<Claim> PyClaim(module, "Claim");

    // constructors
    PyClaim.def(py::init<>());

    // inspections
    PyClaim.def("get_horizontal", (Claim::Stretch & (Claim::*)()) & Claim::get_horizontal, "Returns the horizontal part of this Claim.");
    PyClaim.def("get_vertical", (Claim::Stretch & (Claim::*)()) & Claim::get_vertical, "Returns the vertical part of this Claim.");
    PyClaim.def("get_width_to_height", &Claim::get_width_to_height, "Returns the min and max ratio constraints, 0 means no constraint, is: 0 <= min <= max < INFINITY");

    // modifications
    PyClaim.def("set_horizontal", &Claim::set_horizontal, "Sets the horizontal Stretch of this Claim.", py::arg("stretch"));
    PyClaim.def("set_vertical", &Claim::set_vertical, "Sets the vertical Stretch of this Claim.", py::arg("stretch"));
    PyClaim.def("add_horizontal", &Claim::add_horizontal, "In-place, horizontal addition operator for Claims.", py::arg("other"));
    PyClaim.def("add_vertical", &Claim::add_vertical, "In-place, vertical addition operator for Claims.", py::arg("other"));
    PyClaim.def("set_width_to_height", &Claim::set_width_to_height, "Sets the ratio constraint.", py::arg("ratio_min"), py::arg("ratio_max") = NAN);

    // operators
    PyClaim.def(py::self == py::self);
    PyClaim.def(py::self != py::self);

    // representation
    PyClaim.def("__repr__", [](const Claim& claim) {
        const Claim::Stretch& horizontal = claim.get_horizontal();
        const Claim::Stretch& vertical = claim.get_horizontal();
        const std::pair<float, float> ratio = claim.get_width_to_height();
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
