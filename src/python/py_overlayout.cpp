#include "pybind11/pybind11.h"
namespace py = pybind11;

#include "dynamic/layout/overlayout.hpp"
using namespace notf;

void produce_overlayout(pybind11::module& module, py::detail::generic_type ancestor)
{
    py::class_<Overlayout, std::shared_ptr<Overlayout>> Py_Overlayout(module, "_Overlayout", ancestor);

    module.def("Overlayout", []() -> std::shared_ptr<Overlayout> {
        return Overlayout::create();
    }, "Creates a new Overlayout with an automatically assigned Handle.");

    module.def("Overlayout", [](const Handle handle) -> std::shared_ptr<Overlayout> {
        return Overlayout::create(handle);
    }, "Creates a new Overlayout with an explicitly assigned Handle.", py::arg("handle"));

    Py_Overlayout.def("get_padding", &Overlayout::get_padding, "Padding around the Layout's border.");
    Py_Overlayout.def("get_claim", &Overlayout::get_claim, "The current Claim of this LayoutItem.");

    Py_Overlayout.def("set_padding", &Overlayout::set_padding, "Sets the spacing between items.", py::arg("padding"));
    Py_Overlayout.def("set_claim", &Overlayout::set_claim, "Sets an explicit Claim for this Layout.", py::arg("claim"));

    Py_Overlayout.def("add_item", &Overlayout::add_item, "Adds a new LayoutItem to the front of the Layout.", py::arg("item"));
}
