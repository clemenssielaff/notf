#include "pybind11/pybind11.h"
namespace py = pybind11;

#define NOTF_BINDINGS
#include "core/widget.hpp"
#include "graphics/painter.hpp"
#include "python/docstr.hpp"
#include "python/type_patches.hpp"
using namespace notf;

/* Trampoline Class ***************************************************************************************************/

class PyWidget : public Widget {
public:
    using Widget::Widget;

    virtual ~PyWidget() override;

    /* Trampoline (need one for each virtual function) */
    virtual void paint(Painter& painter) const override
    {
        PYBIND11_OVERLOAD_PURE(
            void, /* Return type */
            Widget, /* Parent class */
            paint, /* Name of function */
            painter /* Argument(s) */
            );
    }
};
PyWidget::~PyWidget() {}

/* Bindings ***********************************************************************************************************/

void produce_widget(pybind11::module& module, py::detail::generic_type Py_LayoutItem)
{
    py::class_<Widget, std::shared_ptr<Widget>, PyWidget> Py_Widget(module, "Widget", Py_LayoutItem);
    patch_type(Py_Widget.ptr());

    Py_Widget.def(py::init<>());

    Py_Widget.def("get_id", &Widget::get_id, DOCSTR("The application-unique ID of this Widget."));
    Py_Widget.def("has_parent", &Widget::has_parent, DOCSTR("Checks if this Item currently has a parent Item or not."));

    Py_Widget.def("get_opacity", &Widget::get_opacity, DOCSTR("Returns the opacity of this Item in the range [0 -> 1]."));
    Py_Widget.def("get_size", &Widget::get_size, DOCSTR("Returns the unscaled size of this Item in pixels."));
    //    Py_Widget.def("get_transform", &Widget::get_transform, "Returns this Item's transformation in the given space.", py::arg("space"));
    Py_Widget.def("get_claim", &Widget::get_claim, DOCSTR("The current Claim of this Widget."));
    Py_Widget.def("get_scissor", &Widget::set_claim, DOCSTR("Returns the Layout used to scissor this Widget."));
    Py_Widget.def("is_visible", &Widget::is_visible, DOCSTR("Checks, if the Item is currently visible."));

    // TODO: complete Widget python bindings (RenderLayers)

    Py_Widget.def("set_opacity", &Widget::set_opacity, DOCSTR("Sets the opacity of this Widget."), py::arg("opacity"));
    Py_Widget.def("set_scissor", &Widget::set_claim, DOCSTR("Sets the new scissor Layout for this Widget."), py::arg("scissor"));
    Py_Widget.def("set_claim", &Widget::set_claim, DOCSTR("Sets a new Claim for this Widget."), py::arg("claim"));

    Py_Widget.def("paint", &Widget::paint, DOCSTR("Paints this Widget onto the screen."), py::arg("painter"));
}
